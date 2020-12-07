#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "linkedList.h"
#include "airplane.h"

// Initialise an airplane list and return it.
struct LinkedList* init_airplane_list(void) {
    return init_linked_list();
}

// Print the given list to the given file.
void print_airplane_list(LinkedList* list, FILE* file) {

    Node* node = list->head;

    while (node->next != NULL) {
        Airplane* this = node->data;
        if (this) {
            fprintf(file, "%s\n", this->id);
        }
        node = node->next;
    }
    fprintf(file, ".\n");
    fflush(file);
}

// Add the given airplane to given list in lexicographic order.
void add_airplane(LinkedList* list, Airplane airplane) {

    Airplane* data = malloc(sizeof(Airplane));
    *data = airplane;
    Node* this = list->head;

    // search for right position to insert the airplane
    while (this && this->next) {

        // case - are we at end of list?
        Node* next = this->next;
        if (next->data == NULL) {
            insert_item(this, data);
            return;
        }

        Airplane* nextAirplane = next->data;

        // compare airplane id to insert and next one in list
        int compare = strcmp(nextAirplane->id, airplane.id);
        if (compare >= 0) {
            insert_item(this, data);
            return;

        } else if (compare < 0) {
            this = this->next;
        }
    }
}

// Remove the airplane with the given id from the list.
void remove_airplane(LinkedList* list, const char* id) {
    Node* node = list->head;

    while (node->next != NULL) {

        Airplane* this = node->data;

        if (this && !strcmp(id, this->id)) {
            remove_item(node);
            return;
        }
        node = node->next;
    }
}

// Given a list & an id, find the id & return a pointer that airplane.
Airplane* get_airplane(LinkedList* list, const char* id) {

    Node* node = list->head;

    while (node->next) {

        Airplane* this = node->data;

        if (this && !strcmp(id, this->id)) {
            return this;
        }
        node = node->next;
    }
    return NULL;
}
