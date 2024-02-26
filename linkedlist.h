#include <stddef.h>

struct node {
    void *data; // data of any type
    struct node *next;
};

typedef struct node node;

struct linked_list {
    node *head;
    node *tail;
    size_t length;
};

typedef struct linked_list linked_list;

/* Node functions */
node *make_node(void *data);

/*Linked List functions */
linked_list *make_linked_list();
int append(linked_list *ll, node *nn);
int delete_node(linked_list *ll, size_t index);
