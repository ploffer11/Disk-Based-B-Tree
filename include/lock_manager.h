#pragma once

#include "disk_manager.h"
#include <vector>
#include <map>
#include <mutex>
#include <utility>
#include <queue>
#include <algorithm>
using namespace std;
typedef pair<int, pagenum_t> pii;

class QueryManager
{
private:
    mutex m;
    int trx_id;

public:
    QueryManager();
    int query(int table_id, int64_t key, char *str, bool change, int trx_id);
    void unlock();
};

struct Query
{
    int table_id;
    int64_t key;
    string prev, cur;
    Query(int table_id, int64_t key, string prev, string cur);
    void rollback();
};

struct Transaction
{
    // my id
    int trx_id;

    // trx's query list
    vector<Query> completed_query_list;

    // query's
    int table_id;
    int64_t key;
    char *value;

    Transaction();
    Transaction(int trx_id);
    int run_query(int table_id, int64_t key, char *value);
};

class TransactionManager
{
private:
    int trx_cnt;
    map<int, Transaction> trx_map;
    mutex mtx1, mtx2;

public:
    TransactionManager();
    // make trx and get unique id
    int begin_trx();

    // end trx(delete all lock) and return trx's end state
    int end_trx(int trx_id);

    int query_trx(int table_id, int64_t key, char *str, bool change, int trx_id);

    bool rollback_trx(int trx_id);
};

class LockManager
{
private:
    // table_id, key -> s lock cnt, s lock,  x lock, x lock trx_id
    map<pair<int, int64_t>, tuple<int, mutex, mutex, int>> total_lock_map;

    // trx_id -> all lock
    map<int, vector<tuple<int, int64_t, bool>>> trx_lock_map;

    // mutex
    mutex mtx1, mtx2;

    bool lock_mutex_timeout(mutex &m, int timeout);

public:
    // add lock
    bool add_lock(int trx_id, int table_id, int64_t key, bool xlock);

    void end_trx(int trx_id);
};

extern TransactionManager trx_manager;
extern LockManager lock_manager;
