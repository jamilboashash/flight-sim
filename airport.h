#ifndef SRC_AIRPORT_H
#define SRC_AIRPORT_H

#include <stdio.h>
#include "linkedList.h"

typedef struct LinkedList* AirportList;

typedef struct {
    const char* id;
    const char* port;
    const char* info;
} Airport;


struct LinkedList* init_airport_list(void);
void add_airport(LinkedList* list, Airport airport);
Airport* get_airport(LinkedList* list, const char* id);
void remove_airport(LinkedList* list, const char* id);
void print_airport_list(LinkedList* list, FILE* file);

#endif //SRC_AIRPORT_H
