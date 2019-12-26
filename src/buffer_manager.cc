#include "common.h"

int bf_size, total_size;
Buffer *buffer;
map<pii, int> collision;

int init_buffer(int num_buf)
{
    bf_size = num_buf;
    total_size = num_buf + COLLISION_SIZE;
    buffer = (Buffer *)malloc(sizeof(Buffer) * total_size);

    if (!buffer)
        return FUNCTION_FAIL;

    memset(buffer, 0, sizeof(Buffer) * total_size);
    return FUNCTION_SUCCESS;
}

int shutdown_buffer(void)
{
    for (int i = 1; i <= TOTAL_TABLE; i++)
        destroy_buffer(i);

    free(buffer);
    return true;
};

void write_buffer(int id, pagenum_t page_num)
{
    int idx = index_buffer(id, page_num);
    buffer[idx].dirty = true;
    buffer[idx].pin--;
}

void unpin_buffer(int id, pagenum_t page_num)
{
    int idx = index_buffer(id, page_num);
    buffer[idx].pin--;
}

void assign_buffer(int id, int i, pagenum_t page_num)
{
    READ_PAGE_WITH_ID(id, page_num, buffer[i].page);
    buffer[i].dirty = buffer[i].ref = false;
    buffer[i].pin = 0;
    buffer[i].page_num = page_num;
    buffer[i].table_id = id;
}

int insert_buffer(int id, pagenum_t page_num)
{
    int hash = (id * HASH + page_num) % bf_size;

    // 만약 pin이 있다면 hash collision이 발생한 것
    if (buffer[hash].pin)
    {
        for (int i = bf_size; i < total_size; i++)
        {
            if (!buffer[i].pin)
            {
                if (buffer[i].page_num && buffer[i].dirty)
                    WRITE_PAGE_WITH_ID(buffer[i].table_id, buffer[i].page_num, buffer[i].page);

                if (buffer[i].page_num && buffer[i].table_id)
                    collision.erase(pii(buffer[i].table_id, buffer[i].page_num));

                assign_buffer(id, i, page_num);
                return collision[pii(id, page_num)] = i;
            }
        }
    }

    else
    {
        if (buffer[hash].page_num && buffer[hash].dirty)
        {
            WRITE_PAGE_WITH_ID(buffer[hash].table_id, buffer[hash].page_num, buffer[hash].page);
        }
        assign_buffer(id, hash, page_num);
        return hash;
    }
}

// -1 means not exist
int index_buffer(int id, pagenum_t page_num)
{
    long hash = (id * HASH + page_num) % bf_size;

    if (buffer[hash].page_num == page_num && buffer[hash].table_id == id)
        return hash;

    else
    {
        int ret = collision[pii(id, page_num)];
        return (ret == 0 ? -1 : ret);
    }
}

page_t *find_buffer(int id, pagenum_t page_num, int val)
{
    int idx = index_buffer(id, page_num);

    if (idx >= 0)
    {
        buffer[idx].ref = true;
        buffer[idx].pin += val;
        return &buffer[idx].page;
    }
    else
    {
        idx = insert_buffer(id, page_num);
        buffer[idx].pin += val;
        return &buffer[idx].page;
    }
}

void destroy_buffer(int id)
{
    for (int i = 0; i < total_size; i++)
    {
        if (buffer[i].table_id == id)
        {
            if (buffer[i].dirty)
                WRITE_PAGE_WITH_ID(id, buffer[i].page_num, buffer[i].page)

            buffer[i].dirty = buffer[i].ref = false;
            buffer[i].pin = 0;
            buffer[i].page_num = 0;
            buffer[i].table_id = 0;
        }
    }
}

void free_buffer(int id, pagenum_t page_num)
{
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, HEADER_PAGE_NUM, header_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, page_num, free_page);

    free_page->FREE_PAGE = header_page->FREE_PAGE;
    header_page->FREE_PAGE = page_num;

    WRITE_PAGE_WITH_BUFFER(id, HEADER_PAGE_NUM);
    WRITE_PAGE_WITH_BUFFER(id, page_num);
}