#pragma once

#include "disk_based_bpt.h"
#include "disk_manager.h"
#include "buffer_manager.h"

#define TOTAL_TABLE 10

extern char path[TOTAL_TABLE + 1][200];

void db_print(int id);

int init_db(int num_buf);
int shutdown_db(void);

int open_table(char *pathname);
int join_table(int table_id_1, int table_id_2, char *pathname);
int close_table(int table_id);

int db_insert(int table_id, int64_t key, char *value);
int db_find(int table_id, int64_t key, char *ret_val);
int db_find(int table_id, int64_t key, char *ret_val, int trx_id);
int db_delete(int table_id, int64_t key);
int db_update(int table_id, int64_t key, char *values);
int db_update(int table_id, int64_t key, char *values, int trx_id);

int begin_trx();
int end_trx(int tid);
