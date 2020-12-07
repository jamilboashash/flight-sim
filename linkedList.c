#include <stdlib.h>
#include "linkedList.h"

// Initialise a new node with all elements set to null & return a pointer
// to that node.
Node* init_new_node(void) {
    Node* node = malloc(sizeof(Node));
    node->data = NULL;
    node->next = NULL;
    return node;
}

// Add given item to end of list.
void add_item(LinkedList* list, void* item) {

    Node* current = list->head;

    while (current->next != NULL) {
        current = current->next;
    }

    current->data = item;
    current->next = init_new_node();
}

// Adjust the previous and next pointers and remove this node.
void remove_item(Node* this) {
    this->data = this->next->data;
    Node* nodeToDelete = this->next;
    this->next = this->next->next;
    free(nodeToDelete);
}

// Initialise a linked list with two null nodes & return a pointer to the list
struct LinkedList* init_linked_list(void) {
    struct LinkedList* list = malloc(sizeof(LinkedList));
    list->head = init_new_node();
    list->head->next = init_new_node();
    return list;
}

// Add given item to the here node of the linked list
void insert_item(Node* here, void* item) {
    Node* node = init_new_node();
    node->data = item;
    node->next = here->next;
    here->next = node;
}


