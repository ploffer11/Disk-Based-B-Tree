#include "common.h"

node *init_queue()
{
    node *ret = make_node(123456789);
    return ret;
}

node *make_node(pagenum_t page_num)
{
    node *ret = (node *)malloc(sizeof(node));
    ret->next = NULL;
    ret->page_num = page_num;
    return ret;
}

node *enqueue(pagenum_t page_num, node *queue)
{
    node *temp;
    node *new_node = make_node(page_num);

    while (queue->next)
        queue = queue->next;

    queue->next = new_node;

    return queue;
}

pagenum_t dequeue(node *queue)
{
    if (!queue->next)
        return NULL;
    else
    {
        pagenum_t temp = queue->next->page_num;
        return temp;
    }
}