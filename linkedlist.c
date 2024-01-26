#include <stdlib.h>

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
node *make_node(void *data) {
    node *nn = (node *)malloc(sizeof(node));
    if (nn == NULL) {
        return NULL;
    }

    // Setup the struct
    nn->data = data;
    nn->next = NULL;

    return nn;
}

/* Linked List functions */
linked_list *make_linked_list() {
    linked_list *ll = (linked_list *)malloc(sizeof(linked_list));
    if (ll == NULL) {
        return NULL;
    }

    // Setup the struct
    ll->head = NULL;
    ll->tail = NULL;
    ll->length = 0;

    return ll;
}

int append(linked_list *ll, node *nn) {
    if (ll->head == NULL) {
        ll->head = nn;
        ll->tail = ll->head;
    } else {
        ll->tail->next = nn;
        ll->tail = nn;
    }
    ll->length++;

    return 0;
}

int delete_node(linked_list *ll, size_t index) {
    if (ll->head == NULL || index >= ll->length) {
        return -1;
    }

    node *current_node = ll->head;
    if (index == 0) {
        // Removal at head
        if (current_node != NULL) {
            ll->head = current_node->next;
        } else {
            ll->head = NULL;
            ll->tail = NULL;
        }

        free(current_node->data);
        free(current_node);
    } else {
        for (size_t i = 0; i < index; i++) {
            current_node = current_node->next;
        }

        node *temp = current_node->next;
        current_node->next = temp->next;

        free(temp->data);
        free(temp);

        // Removal at tail
        if (current_node->next == NULL) {
            ll->tail = current_node;
        }
    }
    ll->length--;

    return 0;
}
