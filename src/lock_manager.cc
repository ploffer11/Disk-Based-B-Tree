#include "common.h"

#include <unistd.h>
TransactionManager trx_manager;
LockManager lock_manager;
QueryManager query_manager;

QueryManager::QueryManager() { trx_id = -1; }

int QueryManager::query(int table_id, int64_t key, char *str, bool change, int trx_id)
{   
    int ret;
    if (change)
        ret = db_update(table_id, key, str);
    else
        ret = db_find(table_id, key, str);

    return ret;
}

void QueryManager::unlock()
{
    m.lock();
    this->trx_id = -1;
    m.unlock();
}

Query::Query(int table_id, int64_t key, string prev = "", string cur = "")
    : table_id(table_id), key(key), prev(prev), cur(cur) {}

void Query::rollback()
{
    if (!prev.empty())
        db_update(table_id, key, (char *)prev.c_str());
}

Transaction::Transaction() {}

Transaction::Transaction(int trx_id) : trx_id(trx_id) {}

int Transaction::run_query(int table_id, int64_t key, char *value = nullptr)
{
    this->table_id = table_id;
    this->key = key;
    this->value = value;

    // 하나의 쿼리가 끝날 때까지 무한 반복
    while (true)
    {
        bool lock_flag = lock_manager.add_lock(trx_id, table_id, key, true);

        // lock 획득 성공 - change
        if (lock_flag)
        {
            // record 변경 쿼리
            if (value)
            {
                string prev;

                // prev 가 비어있다 -> db에 key에 해당하는 레코드가 없다 -> FAIL
                if (prev.empty())
                    return FUNCTION_FAIL;

                // prev 가 안 비어있다 -> db에 key에 해당하는 레코드가 있어서 변경을 완료했다 -> SUCCESS
                else
                {
                    string cur(value);
                    completed_query_list.push_back(Query(table_id, key, prev, cur));
                    return FUNCTION_SUCCESS;
                }
            }

            // find 쿼리 - 락을 얻었다면 성공한 것임
            else
            {
                completed_query_list.push_back(Query(table_id, key));
                return FUNCTION_SUCCESS;
            }
        }
        // lock 획득 실패 - DEADLOCK
        else
        {
            bool rollback_flag = trx_manager.rollback_trx(trx_id);
            // rollback 을 하지 않음
            if (!rollback_flag)
                continue;
            // rollback 을 함
            else
            {
                for (auto &q : completed_query_list)
                    run_query(q.table_id, q.key, (char *)(q.cur == "" ? nullptr : q.cur.c_str()));
            }
        }
    }
};

TransactionManager::TransactionManager() {}

// make trx and get unique id
int TransactionManager::begin_trx()
{
    mtx1.lock();
    int trx_id = ++trx_cnt;
    trx_map[trx_id] = Transaction(trx_id);
    return trx_id;
}

// end trx(delete all lock) and return trx's end state
int TransactionManager::end_trx(int trx_id)
{
    if (trx_map.find(trx_id) == trx_map.end())
        return FUNCTION_FAIL;

    trx_map.erase(trx_id);
    mtx1.unlock();
    return FUNCTION_SUCCESS;
}

int TransactionManager::query_trx(int table_id, int64_t key, char *str, bool change, int trx_id)
{
    if (((1 << table_id) & 1))
    {
        lock_manager.add_lock(trx_id, table_id, key, true);
        vector<tuple<int, int64_t, bool>> vt;
        //lock_manager.run_trx(trx_id, vt);
    }
    else
    {
        query_manager.query(table_id, key, str, change, trx_id);
    }
}

bool TransactionManager::rollback_trx(int trx_id)
{
    mtx2.lock();

    auto &trx = trx_map[trx_id];

    // before rollback, test!
    bool lock_flag = lock_manager.add_lock(trx.trx_id, trx.table_id, trx.key, trx.value);

    // lock이 됬다면?
    if (lock_flag)
    {
        mtx2.unlock();
        return false;
    }

    for (int i = trx.completed_query_list.size() - 1; i >= 0; i--)
        trx.completed_query_list[i].rollback();

    mtx2.unlock();
    return true;
}

bool LockManager::lock_mutex_timeout(mutex &m, int timeout = 5000)
{
    // ms 단위
    clock_t start = clock();
    while (clock() - start < timeout)
    {
        if (m.try_lock())
            return true;
    }
    return false;
}

bool LockManager::add_lock(int trx_id, int table_id, int64_t key, bool xlock)
{
    auto &value = total_lock_map[{table_id, key}];

    int &s_lock_cnt = get<0>(value);
    mutex &s_lock = get<1>(value);
    mutex &x_lock = get<2>(value);
    int &x_lock_trx_id = get<3>(value);

    if (x_lock_trx_id == trx_id)
    {
        return true;
    }

    // xlock이면 slock과 xlock을 둘다 탈환해야함
    if (xlock)
    {
        bool s_flag = lock_mutex_timeout(s_lock);
        if (!s_flag)
            return false;

        bool x_flag = lock_mutex_timeout(x_lock);
        if (!x_flag)
        {
            s_lock.unlock();
            return false;
        }
        x_lock_trx_id = trx_id;
    }

    else
    {
        mtx1.lock();

        if (s_lock_cnt == 0)
        {
            bool x_flag = lock_mutex_timeout(x_lock);
            if (!x_flag)
            {
                mtx1.unlock();
                return false;
            }
            x_lock_trx_id = trx_id;
            s_lock.lock();
        }
        s_lock_cnt++;

        mtx1.unlock();
    }

    trx_lock_map[trx_id].push_back(tuple<int, int64_t, bool>(table_id, key, xlock));
    return true;
}

void LockManager::end_trx(int trx_id)
{
    int table_id;
    int64_t key;
    bool xlock;

    vector<pair<int, int64_t>> x_lock_list;
    for (auto &value : trx_lock_map[trx_id])
    {
        tie(table_id, key, xlock) = value;

        if (xlock)
        {
            x_lock_list.push_back({table_id, key});
        }
        else
        {
            auto &value2 = total_lock_map[{table_id, key}];
            int &s_lock_cnt = get<0>(value2);
            mutex &s_lock = get<1>(value2);
            s_lock_cnt--;
            if (s_lock_cnt == 0)
            {
                s_lock.unlock();
                x_lock_list.push_back({table_id, key});
            }
        }

        auto &x = x_lock_list;
        sort(x.begin(), x.end());
        x.resize(unique(x.begin(), x.end()) - x.begin());

        for (auto &p : x_lock_list)
        {
            mutex &x_lock = get<2>(total_lock_map[p]);
            x_lock.unlock();
        }
    }

    trx_lock_map.erase(trx_id);
}
