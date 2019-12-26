#include "common.h"

#define HEADER_ON_BUFFER
#define _HEADER_NOT_ON_BUFFER

keyvalue_t *ptr;
char path[TOTAL_TABLE + 1][200];
pagenum_t root[TOTAL_TABLE + 1];
char temp[200];

int init_db(int num_buf)
{
    return init_buffer(num_buf);
}

int shutdown_db(void)
{
    return shutdown_buffer();
}

int open_table(char *pathname)
{
    int table_id;
    for (int i = 1; i <= TOTAL_TABLE; i++)
    {
        if (path[i][0] == NULL)
        {
            table_id = i;
            break;
        }
    }
    strcpy(path[table_id], pathname);
    init_path(pathname);
    CLOSE;
    db_file_array[table_id] = fopen64(path[table_id], FILE_OPEN_MODE);

#ifdef HEADER_ON_BUFFER
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(table_id, HEADER_PAGE_NUM, header_page);
    READ_UINT64_V2(header_page, ROOT_PAGE_OFFSET, root[table_id]);
#endif

    return table_id;
}

int close_table(int table_id)
{
    if (path[table_id][0] == NULL)
        return FUNCTION_FAIL;

    destroy_buffer(table_id);
    root[table_id] = NULL;
    path[table_id][0] = NULL;
    fclose(db_file_array[table_id]);
    return FUNCTION_SUCCESS;
}

int db_insert(int table_id, int64_t key, char *value)
{
    if (path[table_id][0] == NULL)
        return FUNCTION_FAIL;

    if ((ptr = find_from_tree(table_id, root[table_id], key, false)) != NULL)
    {
        FREE(ptr);
        return FUNCTION_FAIL;
    }
    root[table_id] = insert_into_tree(table_id, root[table_id], key, value);

#ifdef HEADER_ON_BUFFER
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(table_id, HEADER_PAGE_NUM, header_page);
    WRITE_UINT64_V2(header_page, ROOT_PAGE_OFFSET, root[table_id]);
    WRITE_PAGE_WITH_BUFFER(table_id, HEADER_PAGE_NUM);
#endif

    return FUNCTION_SUCCESS;
}

int db_find(int table_id, int64_t key, char *ret_val)
{
    if (!root[table_id])
    {
        return FUNCTION_FAIL;
    }

    keyvalue_t *ret = find_from_tree(table_id, root[table_id], key, false);
    if (!ret)
    {
        return FUNCTION_FAIL;
    }
    else
    {
        strcpy(ret_val, (char *)ret->value);
        FREE(ret);
        return FUNCTION_SUCCESS;
    }
}

int db_update(int table_id, int64_t key, char *value)
{
    if (!root[table_id])
    {
        return FUNCTION_FAIL;
    }

    keyvalue_t *ret = find_from_tree(table_id, root[table_id], key, false);
    if (!ret)
    {
        return FUNCTION_FAIL;
    }
    else
    {
        FREE(ret)
        return change_record(table_id, root[table_id], key, value);
    }
}

int db_find(int table_id, int64_t key, char *ret_val, int trx_id)
{
    return trx_manager.query_trx(table_id, key, ret_val, false, trx_id);
}

int db_update(int table_id, int64_t key, char *values, int trx_id)
{
    return trx_manager.query_trx(table_id, key, values, true, trx_id);
}

int db_delete(int table_id, int64_t key)
{
    if (!root[table_id] || (ptr = find_from_tree(table_id, root[table_id], key, false)) == NULL)
    {
        return FUNCTION_FAIL;
    }

    root[table_id] = delete_from_tree(table_id, root[table_id], key);

#ifdef HEADER_ON_BUFFER
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(table_id, HEADER_PAGE_NUM, header_page);
    WRITE_UINT64_V2(header_page, ROOT_PAGE_OFFSET, root[table_id]);
    WRITE_PAGE_WITH_BUFFER(table_id, HEADER_PAGE_NUM);
#endif

    FREE(ptr);
    return FUNCTION_SUCCESS;
}

// return true if pagenum(second) change
bool go_next_record(int id, pii &x, page_t *page)
{
    auto cur_idx = x.first;
    auto cur_page_num = x.second;

    if (page->num_keys - 1 == cur_idx)
    {
        x = pii(0, page->right_sibling);
        return true;
    }
    else
    {
        x.first++;
        return false;
    }
}

int join_table(int table_id_1, int table_id_2, char *pathname)
{
    if (path[table_id_1][0] == NULL || path[table_id_2][0] == NULL)
        return FUNCTION_FAIL;

    FILE *out = fopen64(pathname, "wb+");

    pagenum_t c_page_num_1 = find_left_child(table_id_1, root[table_id_1]);
    pagenum_t c_page_num_2 = find_left_child(table_id_2, root[table_id_2]);

    READ_ONLY_BUT_PIN_PAGE_AFTER_DECLARATION_WITH_BUFFER(table_id_1, c_page_num_1, c_page_1);
    READ_ONLY_BUT_PIN_PAGE_AFTER_DECLARATION_WITH_BUFFER(table_id_2, c_page_num_2, c_page_2);

    auto x = pii(0, c_page_num_1);
    auto y = pii(0, c_page_num_2);

    int64_t x_key, y_key;
    while (true)
    {
        auto prev_page_num_1 = x.second;
        auto prev_page_num_2 = y.second;
        x_key = GET_KEY(c_page_1, x.first);
        y_key = GET_KEY(c_page_2, y.first);

        if (x_key == y_key)
        {
            auto x_ptr = (char *)get_key(c_page_1, x.first) + sizeof(int64_t);
            auto y_ptr = (char *)get_key(c_page_2, y.first) + sizeof(int64_t);
            fprintf(out, "%lld,%s,%lld,%s\n", x_key, x_ptr, y_key, y_ptr);

            if (go_next_record(table_id_1, x, c_page_1))
            {
                if (x.second == NO_RIGHT)
                    break;
                UNPIN_PAGE_WITH_BUFFER(table_id_1, prev_page_num_1);
                READ_ONLY_BUT_PIN_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(table_id_1, x.second, c_page_1);
            }

            if (go_next_record(table_id_2, y, c_page_2))
            {
                if (y.second == NO_RIGHT)
                    break;
                UNPIN_PAGE_WITH_BUFFER(table_id_2, prev_page_num_2);
                READ_ONLY_BUT_PIN_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(table_id_2, y.second, c_page_2);
            }
        }
        else if (x_key < y_key)
        {
            if (go_next_record(table_id_1, x, c_page_1))
            {
                if (x.second == NO_RIGHT)
                    break;
                UNPIN_PAGE_WITH_BUFFER(table_id_1, prev_page_num_1);
                READ_ONLY_BUT_PIN_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(table_id_1, x.second, c_page_1);
            }
        }
        else
        {
            if (go_next_record(table_id_2, y, c_page_2))
            {
                if (y.second == NO_RIGHT)
                    break;
                UNPIN_PAGE_WITH_BUFFER(table_id_2, prev_page_num_2);
                READ_ONLY_BUT_PIN_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(table_id_2, y.second, c_page_2);
            }
        }
    }

    printf("join Table: %s\n", pathname);
    fflush(stdout);
    fclose(out);
    return FUNCTION_SUCCESS;
}

void db_print(int id)
{
    print_tree(id, root[id]);
}

int begin_trx()
{
    return trx_manager.begin_trx();
}

int end_trx(int tid)
{
    return trx_manager.end_trx(tid);
}