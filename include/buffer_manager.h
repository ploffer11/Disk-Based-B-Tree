#pragma once
#include "disk_manager.h"
#include "disk_based_bpt.h"

typedef struct Buffer
{
    bool dirty, ref;
    int pin, table_id;
    pagenum_t page_num;
    page_t page;
} Buffer;

int init_buffer(int num_buf);
int shutdown_buffer(void);
int insert_buffer(int id, pagenum_t page_num);
int index_buffer(int id, pagenum_t page_num);

page_t *find_buffer(int id, pagenum_t page_num, int val);

void unpin_buffer(int id, pagenum_t page_num);
void write_buffer(int id, pagenum_t page_num);
void increase_ptr(int id);
void assign_buffer(int id, int i, pagenum_t page_num);
void destroy_buffer(int id);
void free_buffer(int id, pagenum_t page_num);