#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "linkedList.h"
#include "airport.h"

// Initialise an airport list and return it.
struct LinkedList* init_airport_list(void) {
    return init_linked_list();
}

// Print the given list to the given file.
void print_airport_list(LinkedList* list, FILE* file) {

    Node* node = list->head;

    while (node->next != NULL) {
        Airport* this = node->data;
        if (this) {
            fprintf(file, "%s:%s\n", this->id, this->port);
        }
        node = node->next;
    }
    fflush(file);
}

// Adds an airport to the linked list in lexicographic order.
void add_airport(LinkedList* list, Airport airport) {

    Airport* data = malloc(sizeof(Airport));
    *data = airport;
    Node* this = list->head;

    // search for right spot to insert the airport
    while (this && this->next) {

        // case - are we at end of list?
        Node* next = this->next;
        if (next->data == NULL) {
            insert_item(this, data);
            return;
        }

        Airport* nextAirport = next->data;

        // compare airport id to insert and next one in list
        int compare = strcmp(nextAirport->id, airport.id);
        if (compare == 0) {
            //todo double check how to handle case when airports match
            return;

        } else if (compare > 0) {
            insert_item(this, data);
            return;

        } else if (compare < 0) {
            this = this->next;
        }
    }
}

// Remove the airport with the given id from the list.
void remove_airport(LinkedList* list, const char* id) {
    Node* node = list->head;

    while (node->next != NULL) {

        Airport* this = node->data;

        if (this && !strcmp(id, this->id)) {
            remove_item(node);
            return;
        }
        node = node->next;
    }
}

// Given an airport id, find it & return a pointer to that airport in the list
Airport* get_airport(LinkedList* list, const char* id) {

    Node* node = list->head;

    while (node->next) {

        Airport* this = node->data;

        if (this && !strcmp(id, this->id)) {
            return this;
        }
        node = node->next;
    }
    return NULL;
}
