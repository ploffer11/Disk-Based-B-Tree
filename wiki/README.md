# Disk based B+ tree 

## Before reading WIKI

- Disk based B+ tree를 만들기 위해 다음과 같은 .cc 파일을 만들었고, 그에 따라 헤더 파일을 만들어 관리했음을 명시한다.
    - queue.cc
    - lock_manager.cc
    - buffer_manager.cc
    - disk_manager.cc
    - disk_based_bpt.cc
    - disk_based_bpt_api.cc
    - main.cc (test 용)

- 그에 따라, Makefile은 /src 내의 모든 .cc 파일에 대한 오브젝트 파일을 만들도록 수정되어 있음도 미리 알린다.

- 이 WIKI 에서는 main.cc 를 제외하고 다른 .cc 파일의 헤더에 있는 함수를 위주로 설명한다.

- 설명이 장황해지는 것을 막기 위해 각 함수의 critical point만을 적어두었다.

<hr/>

## Project5 Milestone 2

### Lock Manager에 대해

- cascading abort를 막기 위해 Strict-2PL 를 기반으로 작동한다.
    - Stirct 2PL은 lock을 걸기만 하고 풀지 않는다. 
    - 모든 lock은 transaction이 완벽히 끝나 end_trx 이 불려야만 제거된다. 

- conflict-serializable schedule을 지원한다.
    - multi thread 환경에서 돌아가는 것을 목표로 하므로, mutex를 사용한다.
    - lock을 걸어두어 lock이 걸린 page에 접근해야 할 때 thread는 lock이 풀릴 때까지 대기한다
    - Strict-2PL이 conflict-serializable 을 보장한다.

- lock의 종류는 두 가지이다.
    - shared lock과 exclusive lock으로 이루어져 있다.

- deadlock을 detect할 수 있어야 하며, 이를 해결할 수 있어야 한다.
    - 기다리고 있는 상태를 graph로 저장해 두었다가, cycle을 탐지한다.
    - cycle이 탐지되면, 그 cycle 내부의 우선순위가 낮은 transaction을 roll back 한다.

<hr/>

## LockManager.cc

`int QueryManager::query(int table_id, int64_t key, char *str, bool change, int trx_id)`
- QueryManager에게 새로운 쿼리를 추가한다.

`void QueryManager::unlock()`
- QueryManager의 lock을 푼다.

`void Query::rollback()`
- 쿼리를 롤백한다.

`int Transaction::run_query(int table_id, int64_t key, char *value = nullptr)`
- Transaction 내부의 쿼리를 실행한다.

`int TransactionManager::begin_trx()`
- 새로운 Transaction 을 추가하고 번호를 리턴한다.

`int TransactionManager::end_trx(int trx_id)`
- Transaction 을 끝내고 성공 여부를 리턴한다.

`int TransactionManager::query_trx(int table_id, int64_t key, char *str, bool change, int trx_id)`
- TransactionManager에게 trx_id에 해당하는 transaction에 쿼리를 추가한다.

`bool TransactionManager::rollback_trx(int trx_id)`
- TransactionManager에게 trx_id에 해당하는 transaction의 롤백을 요청한다.

`bool LockManager::lock_mutex_timeout(mutex &m, int timeout = 5000)`
- 시간의 길이로 Deadlock이 걸렸는지 판단한다.

`bool LockManager::add_lock(int trx_id, int table_id, int64_t key, bool xlock)`
- LockManager에게 새로운 lock을 추가한다.

`void LockManager::end_trx(int trx_id)`
- Transaction이 끝나며 모든 lock을 해방한다.

<hr/>

## Project4 join에 대해

`int join_table(int table_id_1, int table_id_2, char *pathname);`

- 먼저 이번에 disk_based_bpt.cc 에 새로 추가된 `pagenum_t find_left_child(int id, pagenum_t root)` 를 사용해 가장 왼쪽 리프노드로 이동한다.

- 이 때, 리프의 키가 정렬되어 있음을 생각하면 투포인터를 이용해 단순히 포인터를 움직이는 것 만으로 복잡도 O(N) 의 merge가 가능하다.

- 각 포인터의 key가 같으면 file에 write한 후, 포인터를 움직이면 된다.

- 각 포인터의 key가 다르면, 작은 쪽의 포인터를 한 칸 옮긴다.

- 모든 key를 확인했다면, right_sibling을 이용해 다음 리프로 넘어가고, 없으면 종료한다.

<hr/>

## buffer_manager.cc

### Buffer 의 로직에 대해
    이 buffer는 해싱을 기반으로 작동한다. 원하는 위치에 찾아가 데이터를 확인한 뒤, collision 여부를 판단해 각기 다른 동작을 한다.
    collision이 일어나지 않았을 때, 그냥 그 자리에 넣는다.
    collision이 일어났을 때, collision용 배열로 가 적절한 위치를 찾아서 넣는다.

`int init_buffer(int num_buf);`
- 버퍼를 num_buf 크기의 사이즈로 초기화하는 함수이다.

`int shutdown_buffer(void);`
- 버퍼의 모든 내용물을 비우는 함수이다.
- 이 때, dirty한 부분이 있다면 파일에 출력한다.

`int insert_buffer(int id, pagenum_t page_num);`
- 버퍼에 id와 page_num을 가진 페이지를 삽입하는 함수이다.

`int index_buffer(int id, pagenum_t page_num);`
- 버퍼에 id와 page_num을 가진 페이지가 미리 올라가 있는지 확인하고, 그 위치를 반환하는 함수이다.
- 만약 버퍼에 없을 시, -1을 리턴한다.

`page_t *find_buffer(int id, pagenum_t page_num, int val);`
- 버퍼에 id와 page_num을 가진 페이지의 포인터를 반환하는 함수이다.
- 만약, 없다면 새로 하나 넣은 후 리턴하고 있다면 바로 포인터를 리턴한다.

`void write_buffer(int id, pagenum_t page_num);`
- 버퍼에 무언가 내용물이 바뀌었다고 마킹하는 함수이다.

`void assign_buffer(int id, int i, pagenum_t page_num);`
- 버퍼에 파일 매니저를 통해 내용물을 작성하는 함수이다.

`void destroy_buffer(int id);`
- id에 해당하는 버퍼를 전부 버퍼에서 비우는 함수이다.

<hr/>

## queue.cc

### 이 파일은 queue 구현체를 따로 파일로 관리한 파일이며, 포인터 구조의 큐를 사용하도록 도와준다.

`node *init_queue();`
- queue를 처음에 초기화 하기 위한 함수

`node *make_node(pagenum_t page_num);`
- page_num 을 담은 노드를 동적할당해서 리턴해주는 함수

`node *enqueue(pagenum_t page_num, node *queue);`
- queue에 page_num 을 담은 노드를 만들어 큐에 넣어주는 함수

`pagenum_t dequeue(node *queue);`
- queue에 노드 하나를 제거하고 거기에 들어있던 값을 리턴해주는 함수

<hr/>

## disk_manager.cc 

### 이 파일은 각 페이지를 정의하는 구조체와 다양한 #define 문을 가지고 있다. 구조체는 과제 명세 그대로 짜여 있다.

`pagenum_t file_alloc_page();`
- free page를 만들어 그 free page의 번호를 리턴하는 함수. 
- 만약 free page가 없을 경우 header_alloc_free_page 를 호출해 새로 free page를 만든다.

`pagenum_t get_page_cnt();`
- 현재 총 page의 개수를 세서 리턴해주는 함수

`pagenum_t get_db_num(char *file_path);`
- file의 path를 받아 그 db의 unique한 id를 리턴해주는 함수
- string의 대표적 해싱인 라빈-카프 해싱을 사용해 고유의 번호를 부여하고, 파일에 보관한 후 unique한 id를 리턴한다.

`uint64_t *get_pointer(page_t *page, int idx);`
- page 구조체의 특성상 접근하기 힘든 pointer들을 찾아주는 함수
- internal일 때와 leaf일 때의 동작이 다르다.

`int64_t *get_key(page_t *page, int idx);`
- page 구조체의 특성상 접근하기 힘든 key들을 찾아주는 함수
- internal일 때와 leaf일 때의 동작이 다르다.

`int64_t power(int a, int n);`
- (a^n mod p) 를 O(log n) 에 계산해주는 재귀 함수 

`int64_t hashing(char *str);`
- 인자로 들어온 스트링을 라빈-카프 해싱해 숫자를 리턴해주는 함수

`int initialize(char *file_path);`
- file의 path를 받아 맨 처음에 해야할 동작들을 구현해놓은 함수

`void file_free_page(pagenum_t pagenum);`
- 인자로 들어온 번호를 free page로 만드는 함수 

`void file_read_page(pagenum_t pagenum, page_t *dest);`
- file에서 pagenum에 해당하는 부분을 읽어 dest에 넣어주는 함수

`void file_write_page(pagenum_t pagenum, const page_t *src);`
- file에서 pagenum에 해당하는 부분에 src를 써주는 함수

`void header_alloc_free_page();`
- header에 남은 페이지가 없을 경우, 한 번에 10개의 free page를 할당해 header에 이어주는 함수

<hr/>

## disk_based_bpt.cc

### 이 파일은 bpt.cc 를 기반으로 만들어져 있으며, 적절한 수정을 거쳐 Disk based B+ tree로 작동하게 만들어져 있다. Delayed Merge의 logic도 적절히 구현되어 있다.

`pagenum_t find_left_child(int id, pagenum_t root)`
- 가장 왼쪽 leaf node를 찾아주는 함수

`pagenum_t path_to_root(pagenum_t root, pagenum_t child);`
- child에서 root까지의 길이를 리턴해주는 함수

`void print_tree(pagenum_t root);`
- tree를 queue를 사용해 출력하는 함수

`int get_left_index(pagenum_t parent, pagenum_t left);`
- left index를 반환해주는 함수

`pagenum_t find_leaf(pagenum_t root, int64_t key, bool verbose);`
- 인자로 받은 key가 있는 리프의 pagenum을 리턴해주는 함수

`keyvalue_t *find(pagenum_t root, int64_t key, bool verbose);`
- 인자로 받은 key가 있는 keyvalue를 찾아 주소를 리턴해주는 함수

`int cut(int length);`
- length를 2로 나눈 것을 리턴하는 함수, 단 홀수일 땐 2로 나눈 후 1을 더해 리턴

`keyvalue_t *make_record(int64_t key, char *value);`
- key와 value를 담은 레코드를 할당해 포인터를 리턴하는 함수

`pagenum_t make_internal_page(void);`
- free page 하나를 internal page로 만들어 그 번호를 리턴하는 함수

`pagenum_t make_leaf_page(void);`
- free page 하나를 leaf page로 만들어 그 번호를 리턴하는 함수

`pagenum_t insert_into_leaf_after_splitting(pagenum_t root, pagenum_t leaf, keyvalue_t *pointer);`
- 꽉찬 리프 노드를 split하고 insert하는 함수

`pagenum_t insert_into_leaf(pagenum_t leaf, keyvalue_t *pointer);`
- 아직 여유공간이 있는 리프 노드에 insert하는 함수

`pagenum_t insert_into_node_after_splitting(pagenum_t root, pagenum_t old_node, int left_index, int64_t key, pagenum_t right);`
- 꽉찬 internal 노드를 split하고 insert하는 함수

`pagenum_t insert_into_node(pagenum_t root, pagenum_t n, int left_index, int64_t key, pagenum_t right);`
- 아직 여유 공간이 있는 internal 노드에 insert하는 함수

`pagenum_t insert_into_parent(pagenum_t root, pagenum_t left, int64_t key, pagenum_t right);`
- parent에 left와 right를 잇는 함수

`pagenum_t insert_into_new_root(pagenum_t left, int64_t key, pagenum_t right);`
- 새로운 root에 left와 right를 잇는 함수

`pagenum_t start_new_tree(keyvalue_t *pointer);`
- 새로운 tree를 만드는 함수

`pagenum_t insert(pagenum_t root, int64_t key, char *value);`
- insert를 종합해서 관리해주는 함수

`int get_neighbor_index(pagenum_t n);`
- 어떤 노드의 neighbor index를 구해주는 함수

`pagenum_t coalesce_nodes(pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, int k_prime);`
- 두 노드를 합치는 함수

`pagenum_t redistribute_nodes(pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, int k_prime_index, int k_prime);`
- 두 노드의 키를 재분배하는 함수

`pagenum_t remove_entry_from_node(pagenum_t n, keyvalue_t *pointer);`
- 이 함수는 레코드를 지우고 coalesce_nodes를 호출할지 redistribute_nodes를 호출할지 정하는 함수이다.
- Disk based B+ tree의 delayed merge를 구현하기 위해, 단순히 합칠 수 있다고 합치는 게 아닌, 페이지의 key 개수가 0이 될 때까지는 redistribute_nodes를 호출하다가 key 개수가 0이 됬을 때, coalesce_nodes를 호출함으로서 delayed merge를 구현해 놓았다. 

`pagenum_t adjust_root(pagenum_t root);`
- root를 조정하는 함수

`pagenum_t delete_entry(pagenum_t root, pagenum_t n, keyvalue_t *pointer);`
- 어떤 노드 n에서 keyvalue를 지우는 함수

`pagenum_t delete (pagenum_t root, int64_t key);`
- delete를 종합해서 관리하는 함수

`void destroy_page(pagenum_t root);`
- page를 free page로 만드는 함수

`pagenum_t destroy_tree(pagenum_t root);`
- root를 free page로 만드는 함수

<hr/>

## disk_based_bpt_api.cc

### 이 파일은 과제 명세에 나와있던 API를 구현해놓은 .cc 파일로 특이사항이 없다.

`int init_db(int num_buf);`
- buffer의 사이즈를 초기화하는 함수

`int shutdown_db(void);`
- 모든 buffer를 닫아주는 함수

`int close_table(int table_id);`
- table_id 에 해당하는 buffer를 닫아주는 함수

`int db_insert(int table_id, int64_t key, char *value);`
- 열려 있는 db 파일에 레코드를 삽입하는 함수
- .cc 파일에서 열려 있는 db 파일의 root를 글로벌 변수로 보관하고 있다.

`int db_find(int table_id, int64_t key, char *ret_val);`
- 열려 있는 db 파일에 레코드가 있는지 찾아 ret_val에 담아주는 함수

`int db_delete(int table_id, int64_t key);`
- 열려 있는 db 파일에 레코드를 삭제하는 함수

`int open_table(char *pathname);`
- path에 해당하는 파일을 읽는 API 함수
- 파일이 없을 경우, 새로 만들고 파일이 있을 경우 ROOT를 읽는다.
