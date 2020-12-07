#include <stdlib.h>
#include "mapperProtocol.h"

#define CHUNK_SIZE 80

// Read from file until finding the sentinel. If finding the sentinel, return
// else if hitting \n or EOF keep reading.
const char* parse_str(FILE* file, int sentinel) {

    int pos = 0;
    char* str = malloc(sizeof(char) * CHUNK_SIZE);

    // read until hitting '\n' or EOF
    while (1) {
        int next = fgetc(file);

        if (next == sentinel) {
            str[pos] = '\0';
            return str;

        } else if (next == EOF) {
            return NULL; //todo see spec - how to handle EOF
        }
        if (pos > CHUNK_SIZE - 1) {
            str = realloc(str, sizeof(int) * pos);
        }
        str[pos++] = (char)next;
    }
}

// Read the incoming message/request from mapperIn, initialise the data
// structure that stores the incoming request & return the message.
MapperMsg read_message(FILE* mapperIn) {

    MapperMsg msg;

    if (feof(mapperIn)) {
        msg.type = CONN_CLOSED;
        return msg;
    }

    msg.type = fgetc(mapperIn);

    switch (msg.type) {
        case PORT_REQUEST:
            msg.args.id = parse_str(mapperIn, '\n');
            break;
        case ADD_AIRPORT:
            msg.args.id = parse_str(mapperIn, ':');
            msg.args.port = parse_str(mapperIn, '\n');
            break;
        case INFO_REQUEST:
            parse_str(mapperIn, '\n'); // replace \n with \0
            break;
        case CONN_CLOSED:
        default:
            break;
    }
    return msg;
}
