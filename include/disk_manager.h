#pragma once

#if !defined(TOTAL_TABLE)
#define TOTAL_TABLE 10
#endif

extern FILE *db_file;
extern FILE *db_file_array[TOTAL_TABLE + 1];

typedef uint64_t pagenum_t;

typedef struct keyvalue_t
{
    int64_t key;
    int64_t value[15]; // if it is not leaf, value[0] is next page number
} keyvalue_t;

// in-memory page structure
typedef struct page_t
{
    pagenum_t next_page; // it is next free page number if this page is free page, or parent page number if this page is internal page.
    uint32_t is_leaf;
    uint32_t num_keys;
    uint8_t __RESERVED[104];
    pagenum_t right_sibling; // it is leaf page number if this is not leaf or right sibling if this is leaf
    keyvalue_t keys[31];
} page_t;

pagenum_t file_alloc_page();
pagenum_t file_alloc_page_with_id(int id);
pagenum_t get_page_cnt();

uint64_t *get_pointer(page_t *page, int idx);
int64_t *get_key(page_t *page, int idx);

void init_path(char *file_path);

void file_free_page(pagenum_t pagenum);
void file_read_page(pagenum_t pagenum, page_t *dest);
void file_write_page(pagenum_t pagenum, const page_t *src);

void file_write_page_with_id(int id, pagenum_t pagenum, const page_t *src);
void file_read_page_with_id(int id, pagenum_t pagenum, page_t *dest);
void file_free_page_with_id(int id, pagenum_t pagenum);

void header_alloc_free_page();
