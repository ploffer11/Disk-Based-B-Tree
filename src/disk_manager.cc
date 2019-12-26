#include "common.h"

FILE *db_file = NULL;
FILE *db_file_array[TOTAL_TABLE + 1];

pagenum_t header = 0;
uint64_t dummy;

// Allocate an on-disk page from the free page list

pagenum_t file_alloc_page_with_id(int id)
{
    db_file = db_file_array[id];
    return file_alloc_page();
}

pagenum_t file_alloc_page()
{
    READ_PAGE_AFTER_DECLARATION(header, header_page);
    pagenum_t free_page_index = header_page.FREE_PAGE;

    if (!free_page_index)
        header_alloc_free_page();

    // Get the first free page, and read next free page number then modify header's first free page number to it.
    file_read_page(header, &header_page);
    free_page_index = header_page.FREE_PAGE;

    READ_PAGE_AFTER_DECLARATION(free_page_index, free_page);
    header_page.FREE_PAGE = free_page.next_page;

    WRITE_PAGE(header, header_page);
    return free_page_index;
}

// Free an on-disk page to the free page list

void file_free_page_with_id(int id, pagenum_t pagenum)
{
    db_file = db_file_array[id];
    file_free_page(pagenum);
}

void file_free_page(pagenum_t pagenum)
{
    if (!db_file)
        return;

    READ_PAGE_AFTER_DECLARATION(header, header_page);
    READ_PAGE_AFTER_DECLARATION(pagenum, free_page);

    // Read the page whose number is given, then set next free page number as header's first free page number.
    free_page.next_page = header_page.FREE_PAGE;

    // Set header's first free page number as given number.
    header_page.FREE_PAGE = pagenum;

    file_write_page(pagenum, &free_page);
    file_write_page(header, &header_page);
    return;
}

void file_read_page_with_id(int id, pagenum_t pagenum, page_t *dest)
{
    db_file = db_file_array[id];
    file_read_page(pagenum, dest);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t *dest)
{
    if (!db_file)
        return;

    fseeko64(db_file, PAGE_SIZE * pagenum, SEEK_SET);
    fread(dest, PAGE_SIZE, 1, db_file);

    return;
}

void file_write_page_with_id(int id, pagenum_t pagenum, const page_t *src)
{
    db_file = db_file_array[id];
    file_write_page(pagenum, src);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t *src)
{
    if (!db_file)
        return;
    fseeko64(db_file, PAGE_SIZE * pagenum, SEEK_SET);
    fwrite(src, PAGE_SIZE, 1, db_file);

    fflush(db_file);
    return;
}

// Return current the number of page
pagenum_t get_page_cnt()
{
    fseeko64(db_file, 0, SEEK_END);
    return (ftello64(db_file) / PAGE_SIZE);
}

// Alloc header FREE_PAGE_CNT free pages
void header_alloc_free_page()
{
    READ_PAGE_AFTER_DECLARATION(header, header_page);

    pagenum_t page_cnt = get_page_cnt();
    pagenum_t page_num = page_cnt + FREE_PAGE_CNT;

    header_page.FREE_PAGE = page_cnt;
    WRITE_UINT64(header_page, PAGE_NUM_OFFSET, page_num);
    WRITE_PAGE(header, header_page);

    for (int i = page_cnt; i < page_cnt + FREE_PAGE_CNT; i++)
    {
        page_t new_free_page;
        new_free_page.next_page = (i == page_cnt + FREE_PAGE_CNT - 1 ? 0 : i + 1);
        WRITE_PAGE(i, new_free_page);
    }
}

// Open path's database file and return unique db's number
void init_path(char *file_path)
{
    if (file_path == NULL)
        file_path = MY_DB_PATH;

    db_file = fopen64(file_path, FILE_OPEN_MODE);

    if (!db_file)
        db_file = fopen64(file_path, "wb+");

    int cnt = get_page_cnt();
    if (cnt == 0)
    {
        READ_PAGE_AFTER_DECLARATION(header, header_page);
        uint64_t root = 0;
        WRITE_UINT64(header_page, ROOT_PAGE_OFFSET, root);
        WRITE_PAGE(header, header_page);
        header_alloc_free_page();
    }
}

int64_t *get_key(page_t *page, int idx)
{
    if (page->is_leaf)
        return &page->keys[idx].key;
    else
    {
        int i = idx / 8;
        int j = (idx - i * 8) * 2 - 1;
        if (j == -1)
            return &page->keys[i].key;
        else
            return &page->keys[i].value[j];
    }
}

uint64_t *get_pointer(page_t *page, int idx)
{
    if (page->is_leaf)
        return (uint64_t *)&dummy;

    else
    {
        if (idx == 0)
            return (uint64_t *)&page->right_sibling;
        else
        {
            int i = (idx - 1) / 8;
            int j = (idx - 1 - i * 8) * 2;
            return (uint64_t *)&page->keys[i].value[j];
        }
    }
}
