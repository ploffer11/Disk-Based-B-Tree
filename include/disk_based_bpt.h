#pragma once
#include "disk_manager.h"

pagenum_t find_left_child(int id, pagenum_t root);
pagenum_t path_to_root(int id, pagenum_t root, pagenum_t child);
pagenum_t make_internal_page(int id);
pagenum_t make_leaf_page(int id);
pagenum_t insert_into_leaf_after_splitting(int id, pagenum_t root, pagenum_t leaf, keyvalue_t *pointer);
pagenum_t insert_into_leaf(int id, pagenum_t leaf, keyvalue_t *pointer);
pagenum_t insert_into_node_after_splitting(int id, pagenum_t root, pagenum_t old_node, int left_index, int64_t key, pagenum_t right);
pagenum_t insert_into_node(int id, pagenum_t root, pagenum_t n, int left_index, int64_t key, pagenum_t right);
pagenum_t insert_into_parent(int id, pagenum_t root, pagenum_t left, int64_t key, pagenum_t right);
pagenum_t insert_into_new_root(int id, pagenum_t left, int64_t key, pagenum_t right);
pagenum_t start_new_tree(int id, keyvalue_t *pointer);
pagenum_t insert_into_tree(int id, pagenum_t root, int64_t key, char *value);
pagenum_t destroy_tree(int id, pagenum_t root);
pagenum_t coalesce_nodes(int id, pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, int k_prime);
pagenum_t redistribute_nodes(int id, pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, int k_prime_index, int k_prime);
pagenum_t remove_entry_from_node(int id, pagenum_t n, keyvalue_t *pointer);
pagenum_t adjust_root(int id, pagenum_t root);
pagenum_t delete_entry(int id, pagenum_t root, pagenum_t n, keyvalue_t *pointer);
pagenum_t delete_from_tree(int id, pagenum_t root, int64_t key);
pagenum_t find_leaf(int id, pagenum_t root, int64_t key, bool verbose);
pagenum_t path_to_root(int id, pagenum_t root, pagenum_t child);

keyvalue_t *make_record(int id, int64_t key, char *value);
keyvalue_t *find_from_tree(int id, pagenum_t root, int64_t key, bool verbose);

int change_record(int id, pagenum_t root, int64_t key, char *value);
int get_left_index(int id, pagenum_t parent, pagenum_t left);
int cut(int length);
int get_neighbor_index(int id, pagenum_t n);

void destroy_page(int id, pagenum_t root);
void print_tree(int id, pagenum_t root);
