#ifndef SRC_MAPPERPROTOCOL_H
#define SRC_MAPPERPROTOCOL_H

#include <stdio.h>
#include "airport.h"



typedef enum {
    PORT_REQUEST = '?',
    ADD_AIRPORT = '!',
    INFO_REQUEST = '@',
    CONN_CLOSED = EOF
} MapperMsgType;

typedef struct {
    const char* id;
    const char* port;
} MapperMsgArgs;

typedef struct {
    MapperMsgType type;
    MapperMsgArgs args;
} MapperMsg;

const char* parse_str(FILE* file, int sentinel);
MapperMsg read_message(FILE* mapperIn);

#endif //SRC_MAPPERPROTOCOL_H
