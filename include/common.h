#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <tuple>
#include <mutex>
#include <thread>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "buffer_manager.h"
#include "disk_based_bpt_api.h"
#include "disk_based_bpt.h"
#include "disk_manager.h"
#include "queue.h"
#include "lock_manager.h"

typedef pair<int, pagenum_t> pii;

#define OPTIMIZE_OPTION
#ifdef OPTIMIZE_OPTION
#pragma GCC optimize("Ofast")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
#endif

#define NULL 0
#define NULLPTR nullptr
//#define NULLPTR (void *)0

#define LEAF_ORDER 32
#define INTERNAL_ORDER 249

#define COLLISION_SIZE 1000
#define HASH 1000000007LL
#define PRIME 1000000007LL

#define FUNCTION_SUCCESS 0
#define FUNCTION_FAIL 1

#define PAGE_SIZE 4096LL
#define ROOT_PAGE_OFFSET 8
#define PAGE_NUM_OFFSET 16

#define FREE_PAGE next_page
#define PARENT_PAGE next_page

#define _FILE_OFFSET_BITS 64

#define HEADER_PAGE_NUM 0

#define FREE_PAGE_CNT 10
#define NO_PARENT 0
#define NO_RIGHT 0

#define TRX_FIND 1
#define TRX_CHANGE 2

#define MY_DB_PATH "database.db"
#define FILE_OPEN_MODE "rb+"

#define FREE(ptr)  \
    if (ptr)       \
        free(ptr); \
    ptr = NULLPTR;

#define READ_PAGE_AFTER_DECLARATION(page_num, page) \
    page_t page;                                    \
    file_read_page(page_num, &page);

#define WRITE_PAGE(page_num, page) file_write_page(page_num, &page);
#define WRITE_PAGE_WITH_ID(id, page_num, page) file_write_page_with_id(id, page_num, &page);

#define READ_PAGE(page_num, page) file_read_page(page_num, &page);
#define READ_PAGE_WITH_ID(id, page_num, page) file_read_page_with_id(id, page_num, &page);

#define READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page_t *page = find_buffer(id, page_num, 1);

#define READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page_t *page = find_buffer(id, page_num, 0);

#define READ_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page = find_buffer(id, page_num, 1);

#define READ_ONLY_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page = find_buffer(id, page_num, 0);

#define READ_ONLY_BUT_PIN_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page_t *page = find_buffer(id, page_num, 1);

#define READ_ONLY_BUT_PIN_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(id, page_num, page) \
    page = find_buffer(id, page_num, 1);

#define UNPIN_PAGE_WITH_BUFFER(id, page_num) \
    unpin_buffer(id, page_num);

#define WRITE_PAGE_WITH_BUFFER(id, page_num) \
    write_buffer(id, page_num);

#define KEY_MEMCPY(page1, page2, i, j) memcpy(get_key(page1, i), get_key(page2, j), (page1->is_leaf ? sizeof(keyvalue_t) : sizeof(int64_t)));

#define GET_POINTER(page, idx) (*(get_pointer(page, idx)))
#define GET_KEY(page, idx) (*(get_key(page, idx)))
#define GET_VALUE(page, idx) ((char *)get_key(page, idx) + sizeof(int64_t))

#define WRITE_UINT64(page, offset, num) memcpy((char *)&page + offset, &num, sizeof(uint64_t));
#define READ_UINT64(page, offset, num) num = *((uint64_t *)((char *)&page + offset));

#define WRITE_UINT64_V2(page, offset, num) memcpy((char *)page + offset, &num, sizeof(uint64_t));
#define READ_UINT64_V2(page, offset, num) num = *((uint64_t *)((char *)page + offset));

#define GLOBAL_INIT(id) \
    db_file = fopen64(path[id], FILE_OPEN_MODE);

#define CLOSE fclose(db_file);
