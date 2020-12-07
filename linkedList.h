//
// Created by jmb on 25/5/20.
// Lexicographically sorted linked list
//

#ifndef SRC_LINKEDLIST_H
#define SRC_LINKEDLIST_H

typedef struct Node Node;

// a Node contains some data & a pointer to the next node
struct Node {
    void* data;
    Node* next;
};

// LinkedList is a pointer to the head Node
typedef struct LinkedList {
    Node* head;
} LinkedList;

struct LinkedList* init_linked_list(void);
void add_item(LinkedList* head, void* item);
void insert_item(Node* here, void* item);
void remove_item(Node* head);


#endif //SRC_LINKEDLIST_H
