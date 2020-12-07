#ifndef SRC_AIRPLANE_H
#define SRC_AIRPLANE_H

#include <stdio.h>
#include "linkedList.h"

typedef struct LinkedList* AirplaneList;

typedef struct {
    const char* id;
} Airplane;


struct LinkedList* init_airplane_list(void);
void add_airplane(LinkedList* list, Airplane airplane);
Airplane* get_airplane(LinkedList* list, const char* id);
void remove_airplane(LinkedList* list, const char* id);
void print_airplane_list(LinkedList* list, FILE* file);


#endif //SRC_AIRPLANE_H
