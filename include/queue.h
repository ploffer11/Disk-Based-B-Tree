#pragma once

#include "disk_manager.h"
#include "disk_based_bpt.h"

typedef struct node
{
    pagenum_t page_num;
    struct node *next;
} node;

node *init_queue();
node *make_node(pagenum_t page_num);
node *enqueue(pagenum_t page_num, node *queue);

pagenum_t dequeue(node *queue);