#include "common.h"

node *q = NULLPTR;

pagenum_t find_left_child(int id, pagenum_t root)
{
    pagenum_t c = root;
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, c, c_page);

    while (!c_page->is_leaf)
    {
        c = GET_POINTER(c_page, 0);
        READ_ONLY_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(id, c, c_page);
    }

    return c;
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
pagenum_t path_to_root(int id, pagenum_t root, pagenum_t child)
{
    int length = 0;
    pagenum_t c = child;
    while (c != root)
    {
        READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, c, c_page);
        c = c_page->PARENT_PAGE;

        length++;
    }
    return length;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */

void print_tree(int id, pagenum_t root)
{
    FILE *file = fopen("output.txt", "a+");
    fprintf(file, "table id: %d\n", id);
    pagenum_t n = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root == NULL)
    {
        fprintf(file, "Empty tree.\n");
        return;
    }
    q = init_queue();
    q = enqueue(root, q);
    while (q->next != NULL)
    {
        fflush(file);
        n = dequeue(q);
        q = q->next;
        READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);
        new_rank = path_to_root(id, root, n);
        if (new_rank != rank)
        {
            rank = new_rank;
            fprintf(file, "\n");
        }
        fprintf(file, "<%lld> ", n);

        for (i = 0; i < n_page->num_keys; i++)
        {
            if (n_page->is_leaf)
                fprintf(file, "%s ", GET_VALUE(n_page, i));
            else
                fprintf(file, "%lld ", GET_KEY(n_page, i));
        }

        if (!n_page->is_leaf)
            for (i = 0; i <= n_page->num_keys; i++)
                enqueue(GET_POINTER(n_page, i), q);

        fprintf(file, "| ");
    }
    fprintf(file, "\n");
    fflush(file);
    fclose(file);
}

int get_left_index(int id, pagenum_t parent, pagenum_t left)
{
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
    int left_index = 0;
    while (left_index <= parent_page->num_keys &&
           GET_POINTER(parent_page, left_index) != left)
        left_index++;
    return left_index;
}

pagenum_t find_leaf(int id, pagenum_t root, int64_t key, bool verbose)
{
    int i = 0;
    pagenum_t c = root;
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, c, c_page);
    if (c == NULL || c_page->num_keys == 0)
    {
        return c;
    }

    while (!c_page->is_leaf)
    {
        i = 0;
        while (i < c_page->num_keys)
        {
            if (key >= GET_KEY(c_page, i))
                i++;
            else
                break;
        }
        c = GET_POINTER(c_page, i);
        READ_ONLY_PAGE_WITHOUT_DECLARATION_WITH_BUFFER(id, c, c_page);
    }

    return c;
}

/* Finds and returns the record to which
 * a key refers.
 */
keyvalue_t *find_from_tree(int id, pagenum_t root, int64_t key, bool verbose)
{
    int i = 0;
    pagenum_t c = find_leaf(id, root, key, verbose);
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, c, c_page);
    for (i = 0; i < c_page->num_keys; i++)
    {
        if (GET_KEY(c_page, i) == key)
            break;
    }
    if (i == c_page->num_keys)
        return NULLPTR;
    else
    {
        keyvalue_t *ret = (keyvalue_t *)malloc(sizeof(keyvalue_t));
        memcpy(ret, &c_page->keys[i], sizeof(keyvalue_t));
        return ret;
    }
}

int change_record(int id, pagenum_t root, int64_t key, char *value)
{
    pagenum_t c = find_leaf(id, root, key, false);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, c, c_page);

    int i;

    for (i = 0; i < c_page->num_keys; i++)
        if (GET_KEY(c_page, i) == key)
            break;

    if (i == c_page->num_keys)
        return FUNCTION_FAIL;
    else
    {
        strcpy(GET_VALUE(c_page, i), value);
        WRITE_PAGE_WITH_BUFFER(id, c);
        return FUNCTION_SUCCESS;
    }
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut(int length)
{
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */

keyvalue_t *make_record(int id, int64_t key, char *value)
{
    keyvalue_t *new_record = (keyvalue_t *)malloc(sizeof(keyvalue_t));
    int size = strlen(value) + 1;
    if (size > 120)
    {
        perror("Too large record.");
        exit(EXIT_FAILURE);
    }

    if (new_record == NULL)
    {
        perror("Record creation.");
        exit(EXIT_FAILURE);
    }
    else
    {
        new_record->key = key;
        memcpy(new_record->value, value, size);
    }
    return new_record;
}

pagenum_t make_internal_page(int id)
{
    pagenum_t new_page_num = file_alloc_page_with_id(id);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, new_page_num, new_page);

    new_page->is_leaf = 0;
    new_page->num_keys = 0;
    new_page->right_sibling = NO_RIGHT;
    new_page->PARENT_PAGE = NO_PARENT;

    WRITE_PAGE_WITH_BUFFER(id, new_page_num);
    return new_page_num;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
pagenum_t make_leaf_page(int id)
{
    pagenum_t new_page_num = file_alloc_page_with_id(id);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, new_page_num, new_page);

    new_page->is_leaf = 1;
    new_page->num_keys = 0;
    new_page->right_sibling = NO_RIGHT;
    new_page->PARENT_PAGE = NO_PARENT;

    WRITE_PAGE_WITH_BUFFER(id, new_page_num);
    return new_page_num;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
pagenum_t insert_into_leaf(int id, pagenum_t leaf, keyvalue_t *pointer)
{
    int i, insertion_point;
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, leaf, leaf_page);

    insertion_point = 0;
    while (insertion_point < leaf_page->num_keys &&
           GET_KEY(leaf_page, insertion_point) < pointer->key)
        insertion_point++;

    for (i = leaf_page->num_keys; i > insertion_point; i--)
        leaf_page->keys[i] = leaf_page->keys[i - 1];

    leaf_page->keys[insertion_point] = *pointer;
    leaf_page->num_keys++;

    WRITE_PAGE_WITH_BUFFER(id, leaf);
    FREE(pointer);
    return leaf;
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's ORDER, causing the leaf to be split
 * in half.
 */
pagenum_t insert_into_leaf_after_splitting(int id, pagenum_t root, pagenum_t leaf, keyvalue_t *pointer)
{
    pagenum_t new_leaf = make_leaf_page(id);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, leaf, leaf_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, new_leaf, new_leaf_page);

    keyvalue_t *temp_keys;
    int64_t new_key;
    int insertion_index, split, i, j;

    temp_keys = (keyvalue_t *)malloc(LEAF_ORDER * sizeof(keyvalue_t));
    if (temp_keys == NULL)
    {
        perror("Temporary keys array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < LEAF_ORDER - 1 && GET_KEY(leaf_page, insertion_index) < pointer->key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf_page->num_keys; i++, j++)
    {
        if (j == insertion_index)
            j++;

        temp_keys[j] = leaf_page->keys[i];
    }

    temp_keys[insertion_index] = *pointer;
    leaf_page->num_keys = 0;

    split = cut(LEAF_ORDER - 1);

    for (i = 0; i < split; i++)
    {
        leaf_page->keys[i] = temp_keys[i];
        leaf_page->num_keys++;
    }

    for (i = split, j = 0; i < LEAF_ORDER; i++, j++)
    {
        new_leaf_page->keys[j] = temp_keys[i];
        new_leaf_page->num_keys++;
    }

    new_leaf_page->right_sibling = leaf_page->right_sibling;
    leaf_page->right_sibling = new_leaf;

    new_leaf_page->PARENT_PAGE = leaf_page->PARENT_PAGE;
    new_key = GET_KEY(new_leaf_page, 0);

    WRITE_PAGE_WITH_BUFFER(id, new_leaf);
    WRITE_PAGE_WITH_BUFFER(id, leaf);
    FREE(temp_keys);
    FREE(pointer);
    return insert_into_parent(id, root, leaf, new_key, new_leaf);
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
pagenum_t insert_into_node(int id, pagenum_t root, pagenum_t n,
                           int left_index, int64_t key, pagenum_t right)
{
    int i;
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);

    for (i = n_page->num_keys; i > left_index; i--)
    {
        GET_POINTER(n_page, i + 1) = GET_POINTER(n_page, i);
        KEY_MEMCPY(n_page, n_page, i, i - 1);
    }
    GET_POINTER(n_page, left_index + 1) = right;
    GET_KEY(n_page, left_index) = key;
    n_page->num_keys++;

    WRITE_PAGE_WITH_BUFFER(id, n);
    return root;
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the ORDER, and causing the node to split into two.
 */
pagenum_t insert_into_node_after_splitting(int id, pagenum_t root, pagenum_t old_node, int left_index,
                                           int64_t key, pagenum_t right)
{

    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, old_node, old_node_page);
    int i, j, split, k_prime;
    pagenum_t new_node, child;
    pagenum_t *temp_page;
    int64_t *temp_keys;

    /* First create a temporary set of keys and pointers
     * to hold everything in ORDER, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_page = (pagenum_t *)malloc((INTERNAL_ORDER + 1) * sizeof(pagenum_t));
    if (temp_page == NULL)
    {
        exit(EXIT_FAILURE);
        perror("Temporary pointers array for splitting nodes.");
    }
    temp_keys = (int64_t *)malloc((INTERNAL_ORDER) * sizeof(int64_t));
    if (temp_keys == NULL)
    {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node_page->num_keys + 1; i++, j++)
    {
        if (j == left_index + 1)
            j++;
        temp_page[j] = GET_POINTER(old_node_page, i);
    }

    for (i = 0, j = 0; i < old_node_page->num_keys; i++, j++)
    {
        if (j == left_index)
            j++;
        temp_keys[j] = GET_KEY(old_node_page, i);
    }

    temp_page[left_index + 1] = right;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */

    split = cut(INTERNAL_ORDER);
    new_node = make_internal_page(id);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, new_node, new_node_page);
    old_node_page->num_keys = 0;

    for (i = 0; i < split; i++)
    {
        GET_POINTER(old_node_page, i) = temp_page[i];
        GET_KEY(old_node_page, i) = temp_keys[i];
        old_node_page->num_keys++;
    }
    GET_POINTER(old_node_page, i) = temp_page[i];

    k_prime = temp_keys[split];
    for (++i, j = 0; i < INTERNAL_ORDER; i++, j++)
    {
        GET_POINTER(new_node_page, j) = temp_page[i];
        GET_KEY(new_node_page, j) = temp_keys[i];
        new_node_page->num_keys++;
    }
    GET_POINTER(new_node_page, j) = temp_page[i];

    new_node_page->PARENT_PAGE = old_node_page->PARENT_PAGE;
    for (i = 0; i <= new_node_page->num_keys; i++)
    {
        child = GET_POINTER(new_node_page, i);
        READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, child, child_page);
        child_page->PARENT_PAGE = new_node;
        WRITE_PAGE_WITH_BUFFER(id, child);
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    WRITE_PAGE_WITH_BUFFER(id, old_node);
    WRITE_PAGE_WITH_BUFFER(id, new_node);
    FREE(temp_page);
    FREE(temp_keys);
    return insert_into_parent(id, root, old_node, k_prime, new_node);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
pagenum_t insert_into_parent(int id, pagenum_t root, pagenum_t left, int64_t key, pagenum_t right)
{
    int left_index;
    pagenum_t parent;
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, left, left_page);

    parent = left_page->PARENT_PAGE;

    /* Case: new root_page-> */

    if (parent == NO_PARENT)
        return insert_into_new_root(id, left, key, right);

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = get_left_index(id, parent, left);

    /* Simple case: the new key fits into the node. 
     */

    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
    if (parent_page->num_keys < INTERNAL_ORDER - 1)
        return insert_into_node(id, root, parent, left_index, key, right);

    /* Harder case:  split a node in ORDER 
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(id, root, parent, left_index, key, right);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root_page->
 */
pagenum_t insert_into_new_root(int id, pagenum_t left, int64_t key, pagenum_t right)
{
    pagenum_t root = make_internal_page(id);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, root, root_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, left, left_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, right, right_page);

    GET_KEY(root_page, 0) = key;
    GET_POINTER(root_page, 0) = left;
    GET_POINTER(root_page, 1) = right;
    root_page->num_keys++;
    root_page->PARENT_PAGE = NO_PARENT;
    left_page->PARENT_PAGE = root;
    right_page->PARENT_PAGE = root;

    WRITE_PAGE_WITH_BUFFER(id, root);
    WRITE_PAGE_WITH_BUFFER(id, left);
    WRITE_PAGE_WITH_BUFFER(id, right);

    return root;
}

/* First insertion:
 * start a new tree.
 */
pagenum_t start_new_tree(int id, keyvalue_t *pointer)
{
    pagenum_t root = make_leaf_page(id);
    printf("new root %lld\n", root);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, root, root_page);
    root_page->keys[0] = *pointer;
    root_page->right_sibling = NO_RIGHT;
    root_page->PARENT_PAGE = NO_PARENT;
    root_page->num_keys++;
    WRITE_PAGE_WITH_BUFFER(id, root);
    FREE(pointer);
    return root;
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
pagenum_t insert_into_tree(int id, pagenum_t root, int64_t key, char *value)
{
    keyvalue_t *pointer;
    pagenum_t leaf;

    /* The current implementation ignores
     * duplicates.
     */

    keyvalue_t *temp = find_from_tree(id, root, key, false);

    if (temp != NULL)
    {
        FREE(temp);
        return root;
    }
    FREE(temp);

    /* Create a new record for the
     * value.
     */
    pointer = make_record(id, key, value);

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root == NULL)
        return start_new_tree(id, pointer);

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf = find_leaf(id, root, key, false);

    /* Case: leaf has room for key and pointer.
     */

    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, leaf, leaf_page);
    if (leaf_page->num_keys < LEAF_ORDER - 1)
    {
        leaf = insert_into_leaf(id, leaf, pointer);
        return root;
    }

    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(id, root, leaf, pointer);
}

// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */

int get_neighbor_index(int id, pagenum_t n)
{

    int i;
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);
    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n_page->  
     * If n is the leftmost child, this means
     * return -1.
     */

    pagenum_t parent = n_page->PARENT_PAGE;

    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);

    for (i = 0; i <= parent_page->num_keys; i++)
        if (GET_POINTER(parent_page, i) == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (pagenum_t)n);
    exit(EXIT_FAILURE);
}

pagenum_t remove_entry_from_node(int id, pagenum_t n, keyvalue_t *pointer)
{

    int i, j, num_pointers;
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (GET_KEY(n_page, i) != pointer->key)
        i++;

    j = i + 2;
    for (++i; i < n_page->num_keys; i++)
        KEY_MEMCPY(n_page, n_page, i - 1, i);

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    if (!n_page->is_leaf)
        for (; j <= n_page->num_keys; j++)
            GET_POINTER(n_page, j - 1) = GET_POINTER(n_page, j);

    // One key fewer.
    n_page->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n_page->is_leaf)
        for (i = n_page->num_keys; i < LEAF_ORDER - 1; i++)
            GET_POINTER(n_page, i) = NULL;
    else
        for (i = n_page->num_keys + 1; i < INTERNAL_ORDER; i++)
            GET_POINTER(n_page, i) = NULL;

    WRITE_PAGE_WITH_BUFFER(id, n);
    FREE(pointer);
    return n;
}

pagenum_t adjust_root(int id, pagenum_t root)
{
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, root, root_page);
    pagenum_t new_root;

    /* Case: nonempty root_page->
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root_page->num_keys > 0)
        return root;

    /* Case: empty root_page-> 
     */

    // If it has a child, promote
    // the first (only) child
    // as the new root_page->

    if (!root_page->is_leaf)
    {
        new_root = GET_POINTER(root_page, 0);
        READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, new_root, new_root_page);
        new_root_page->PARENT_PAGE = NO_PARENT;
        WRITE_PAGE_WITH_BUFFER(id, new_root);
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    return new_root;
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
pagenum_t coalesce_nodes(int id, pagenum_t root, pagenum_t n, pagenum_t neighbor,
                         int neighbor_index, int k_prime)
{
    int i, j, neighbor_insertion_index, n_end;
    pagenum_t tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1)
    {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n_page->
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, neighbor, neighbor_page);

    neighbor_insertion_index = neighbor_page->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor_page->
     */

    if (!n_page->is_leaf)
    {

        /* Append k_prime.
         */

        GET_KEY(neighbor_page, neighbor_insertion_index) = k_prime;
        neighbor_page->num_keys++;

        n_end = n_page->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++)
        {
            GET_KEY(neighbor_page, i) = GET_KEY(n_page, j);
            GET_POINTER(neighbor_page, i) = GET_POINTER(n_page, j);
            neighbor_page->num_keys++;
            n_page->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        GET_POINTER(neighbor_page, i) = GET_POINTER(n_page, j);

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor_page->num_keys + 1; i++)
        {
            tmp = GET_POINTER(neighbor_page, i);
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, tmp, tmp_page);
            tmp_page->PARENT_PAGE = neighbor;
            WRITE_PAGE_WITH_BUFFER(id, tmp);
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor_page->
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor_page->
     */

    else
    {
        for (i = neighbor_insertion_index, j = 0; j < n_page->num_keys; i++, j++)
        {
            KEY_MEMCPY(neighbor_page, n_page, i, j);
            neighbor_page->num_keys++;
        }
        neighbor_page->right_sibling = n_page->right_sibling;
    }
    WRITE_PAGE_WITH_BUFFER(id, n);
    WRITE_PAGE_WITH_BUFFER(id, neighbor);
    keyvalue_t *pointer = make_record(id, k_prime, (char *)"X");
    root = delete_entry(id, root, n_page->PARENT_PAGE, pointer);
    destroy_page(id, n);
    FREE(pointer);
    return root;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
pagenum_t redistribute_nodes(int id, pagenum_t root, pagenum_t n, pagenum_t neighbor,
                             int neighbor_index, int k_prime_index, int k_prime)
{
    int i;
    pagenum_t tmp, parent;
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);
    READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, neighbor, neighbor_page);

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1)
    {
        if (!n_page->is_leaf)
            GET_POINTER(n_page, n_page->num_keys + 1) = GET_POINTER(n_page, n_page->num_keys);
        for (i = n_page->num_keys; i > 0; i--)
        {
            KEY_MEMCPY(n_page, n_page, i, i - 1);
            GET_POINTER(n_page, i) = GET_POINTER(n_page, i - 1);
        }
        if (!n_page->is_leaf)
        {
            GET_POINTER(n_page, 0) = GET_POINTER(neighbor_page, neighbor_page->num_keys);

            tmp = GET_POINTER(n_page, 0);
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, tmp, tmp_page);
            tmp_page->PARENT_PAGE = n;

            GET_POINTER(neighbor_page, neighbor_page->num_keys) = NULL;

            GET_KEY(n_page, 0) = k_prime;
            parent = n_page->PARENT_PAGE;
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
            GET_KEY(parent_page, k_prime_index) = GET_KEY(neighbor_page, neighbor_page->num_keys - 1);

            WRITE_PAGE_WITH_BUFFER(id, tmp);
            WRITE_PAGE_WITH_BUFFER(id, parent);
        }
        else
        {
            GET_POINTER(n_page, 0) = GET_POINTER(neighbor_page, neighbor_page->num_keys - 1);
            GET_POINTER(neighbor_page, neighbor_page->num_keys - 1) = NULL;
            KEY_MEMCPY(n_page, neighbor_page, 0, neighbor_page->num_keys - 1);
            parent = n_page->PARENT_PAGE;
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
            KEY_MEMCPY(parent_page, n_page, k_prime_index, 0);

            WRITE_PAGE_WITH_BUFFER(id, parent);
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else
    {
        if (n_page->is_leaf)
        {
            KEY_MEMCPY(n_page, neighbor_page, n_page->num_keys, 0);
            //GET_KEY(n_page, n_page->num_keys) = GET_KEY(neighbor_page, 0);
            GET_POINTER(n_page, n_page->num_keys) = GET_POINTER(neighbor_page, 0);
            parent = n_page->PARENT_PAGE;
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
            KEY_MEMCPY(parent_page, neighbor_page, k_prime_index, 1);
            //GET_KEY(parent_page, k_prime_index) = GET_KEY(neighbor_page, 1);
            WRITE_PAGE_WITH_BUFFER(id, parent);
        }
        else
        {
            GET_KEY(n_page, n_page->num_keys) = k_prime;
            GET_POINTER(n_page, n_page->num_keys + 1) = GET_POINTER(neighbor_page, 0);

            tmp = GET_POINTER(n_page, n_page->num_keys + 1);
            parent = n_page->PARENT_PAGE;

            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, tmp, tmp_page);
            READ_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);

            tmp_page->PARENT_PAGE = n;
            GET_KEY(parent_page, k_prime_index) = GET_KEY(neighbor_page, 0);

            WRITE_PAGE_WITH_BUFFER(id, tmp);
            WRITE_PAGE_WITH_BUFFER(id, parent);
        }
        for (i = 0; i < neighbor_page->num_keys - 1; i++)
        {
            KEY_MEMCPY(neighbor_page, neighbor_page, i, i + 1);
            GET_POINTER(neighbor_page, i) = GET_POINTER(neighbor_page, i + 1);
        }
        if (!n_page->is_leaf)
            GET_POINTER(neighbor_page, i) = GET_POINTER(neighbor_page, i + 1);
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n_page->num_keys++;
    neighbor_page->num_keys--;

    WRITE_PAGE_WITH_BUFFER(id, n);
    WRITE_PAGE_WITH_BUFFER(id, neighbor);

    return root;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
pagenum_t delete_entry(int id, pagenum_t root, pagenum_t n, keyvalue_t *pointer)
{
    int min_keys;
    pagenum_t neighbor, parent;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(id, n, pointer);
    /* Case:  deletion from the root_page-> 
     */

    if (n == root)
        return adjust_root(id, root);

    /* Case:  deletion from a node below the root_page->
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, n, n_page);
    min_keys = n_page->is_leaf ? cut(LEAF_ORDER - 1) : cut(INTERNAL_ORDER) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    //if (n_page->num_keys >= min_keys - 3)
    //if (n_page->num_keys > 10)
    if (n_page->num_keys >= 1)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor_page->
     */
    parent = n_page->PARENT_PAGE;
    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, parent, parent_page);
    neighbor_index = get_neighbor_index(id, n);
    k_prime_index = (neighbor_index == -1 ? 0 : neighbor_index);
    k_prime = GET_KEY(parent_page, k_prime_index);
    neighbor = (neighbor_index == -1 ? GET_POINTER(parent_page, 1) : GET_POINTER(parent_page, neighbor_index));

    capacity = n_page->is_leaf ? LEAF_ORDER : INTERNAL_ORDER - 1;

    /* Coalescence. */

    READ_ONLY_PAGE_AFTER_DECLARATION_WITH_BUFFER(id, neighbor, neighbor_page);
    if (neighbor_page->num_keys == 1)
        return coalesce_nodes(id, root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */
    else
        return redistribute_nodes(id, root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}

/* Master deletion function.
 */
pagenum_t delete_from_tree(int id, pagenum_t root, int64_t key)
{

    pagenum_t key_leaf;
    keyvalue_t *key_record;

    key_record = find_from_tree(id, root, key, false);
    key_leaf = find_leaf(id, root, key, false);
    if (key_record != NULL && key_leaf != NULL)
    {
        root = delete_entry(id, root, key_leaf, key_record);
    }
    return root;
}

void destroy_page(int id, pagenum_t root)
{
    free_buffer(id, root);
}

pagenum_t destroy_tree(int id, pagenum_t root)
{
    free_buffer(id, root);
    return NULL;
}
