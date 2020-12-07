#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include "mapperProtocol.h"

#define MIN_ARGC 3
#define PLANE_ID_ARG 1
#define MAPPER_PORT_ARG 2
#define MAX_PORT_NO 65535
#define MIN_PORT_NO 1
#define NO_MAPPER_PORT "-"

// core components of the control
typedef struct {
    const char* id;
    const char* info;
    const char* port;
} Control;

// core components of the roc
typedef struct {
    const char* planeId;
    const char* mapperPort;
    Control* controls; //also acts as roc's log
    FILE* rocOut;
    FILE* rocIn;
    int destCount;
} Roc;

typedef struct sockaddr SockAddr;
typedef struct addrinfo AddrInfo;

// program exit codes
typedef enum {
    NORMAL_OP = 0,
    INV_ARGC = 1,
    INV_MAPPER_PORT = 2,
    MAPPER_REQ = 3,
    CONN_FAILED = 4,
    NO_MAP = 5
    //todo add exit code 6
} Status;

// Given code, print the relevant status message and return the code.
Status print_status(Status code) {
    char* const status[] = {"",
            "Usage: roc id mapper {airports}",
            "Invalid mapper port",
            "Mapper required",
            "Failed to connect to mapper",
            "No map entry for destination",
            "Failed to connect to at least one destination"};
    fprintf(stderr, "%s\n", status[code]);
    return code;
}

// Check arg for any invalid characters. If invalid, exit with code
// status else return arg.
const char* validate_arg(char* arg, Status status) {

    // check if arg contains any invalid characters
    for (int i = 0; i < strlen(arg); ++i) {
        if (arg[i] == '\n' || arg[i] == '\r' || arg[i] == ':') {
            exit(print_status(status));
        }
    }
    return arg;
}

// If dest is a valid port number true else return false.
bool is_a_port(char* dest) {

    char* end;
    int portNo = (int)strtoul(dest, &end, 10);
    if (*end != '\0' || portNo > MAX_PORT_NO || portNo < MIN_PORT_NO) {
        return false;
    }
    return true;
}

// Check if the given arg is a dash or a valid port number. If not valid, exit
// with code INV_MAPPER_PORT, else return the arg.
const char* init_mapper_port(char* arg) {

    // check if it's a dash
    if (!strcmp(arg, "-")) {
        return NO_MAPPER_PORT;

        // check if it's a valid port number
    } else if (!is_a_port(arg)) {
        exit(print_status(INV_MAPPER_PORT));
    }

    return arg;
}

// Initialise the client to connect to mapper and assign the file relevant
// pointers to roc.
void init_client(Roc* roc) {

    if (!strcmp(roc->mapperPort, NO_MAPPER_PORT)) {
        return;
    }

    AddrInfo* ai = 0;
    AddrInfo hints;

    memset(&hints, 0, sizeof(AddrInfo));
    hints.ai_family = AF_INET; // IPv4  for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo("localhost", roc->mapperPort, &hints, &ai);
    if (err) {
        freeaddrinfo(ai);
        exit(print_status(MAPPER_REQ));
    }

    // returns a socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sockfd, (SockAddr*)ai->ai_addr, sizeof(SockAddr))) {
        exit(print_status(CONN_FAILED));
    }

    // we want separate streams (which we can close independently)
    roc->rocOut = fdopen(sockfd, "w");
    roc->rocIn = fdopen(dup(sockfd), "r");
}

// Parse response from file. If it is a semi colon exit with code NO_MAP else
// return response.
const char* read_response(FILE* file) {

    const char* response = parse_str(file, '\n');

    if (!strcmp(response, ";")) {
        exit(print_status(NO_MAP));
    }

    return response;
}

// Given roc, check if dest is a valid port. If it is assign it otherwise
// send the id to request the port from mapper. Initialise all other control
// elements and return control.
Control init_control(Roc* roc, char* dest) {

    validate_arg(dest, MAPPER_REQ);

    Control control;

    if (is_a_port(dest)) {
        // dest is a valid port
        control.id = NULL;
        control.port = dest;

    } else {
        // dest is an id, request its port from mapper
        if (!strcmp(roc->mapperPort, NO_MAPPER_PORT)) {
            exit(print_status(MAPPER_REQ));
        }

        control.id = dest;
        fprintf(roc->rocOut, "?%s\n", control.id);
        fflush(roc->rocOut);

        control.port = read_response(roc->rocIn);
    }

    control.info = NULL;

    return control;
}

// init DEST_COUNT number of controls
void init_controls(Roc* roc, int argc, char** argv) {

    roc->destCount = argc - MIN_ARGC;

    roc->controls = malloc(sizeof(Control) * roc->destCount);

    if (roc->destCount > 0) {
        for (int i = 0; i < roc->destCount; ++i) {
            roc->controls[i] = init_control(roc, argv[i + (MIN_ARGC)]);
        }
    }
}

// Check program arguments, initialise roc and return a pointer to it.
Roc* init_roc(int argc, char** argv) {

    if (argc < MIN_ARGC) {
        exit(print_status(INV_ARGC));
    }

    Roc* roc = malloc(sizeof(Roc));

    // NORMAL_OP so it prints nothing (but function remains general)
    roc->planeId = validate_arg(argv[PLANE_ID_ARG], NORMAL_OP);
    roc->mapperPort = init_mapper_port(argv[MAPPER_PORT_ARG]);
    init_client(roc);
    init_controls(roc, argc, argv);

    return roc;
}

// Initialise a client connection to communicate with control with
// port destPort. Assign the file pointers to roc for later communication.
void init_control_client(Roc* roc, const char* destPort) {

    AddrInfo* ai = 0;
    AddrInfo hints;

    memset(&hints, 0, sizeof(AddrInfo));
    hints.ai_family = AF_INET; // IPv4  for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo("localhost", destPort, &hints, &ai);
    if (err) {
        return;
    }

    // returns a socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (SockAddr*)ai->ai_addr, sizeof(SockAddr))) {
        return;
    }

    // we want separate streams (which we can close independently)
    roc->rocOut = fdopen(sockfd, "w");
    roc->rocIn = fdopen(dup(sockfd), "r");
}

// Connect to destPort, send the airplane id. Attempt to read back the info
// and return it.
const char* conn_to_dest(Roc* roc, const char* destPort) {

    init_control_client(roc, destPort);

    // send plane id to control
    fprintf(roc->rocOut, "%s\n", roc->planeId);
    fflush(roc->rocOut);

    // read back the destination info
    const char* destInfo = parse_str(roc->rocIn, '\n');

    //todo validate destInfo - if invalid set to null then when printing
    // only print if not null

    return destInfo;
}

// Given roc, connect to each destination in the list of controls and assign
// the control's info.
void conn_to_dests(Roc* roc) {
    //now roc knows all port nos for its destinations

    if (roc->destCount > 0) {
        for (int i = 0; i < roc->destCount; ++i) {
            const char* destPort = roc->controls[i].port;
            roc->controls[i].info = conn_to_dest(roc, destPort);
        }
    }

}

// Print to stdout the list of controls/airports this roc has visited.
void print_log(Roc* roc) {

    for (int i = 0; i < roc->destCount; ++i) {
        fprintf(stdout, "%s\n", roc->controls[i].info);
    }
    fflush(stdout);

}

int main(int argc, char** argv) {

    Roc* roc = init_roc(argc, argv);
    conn_to_dests(roc);
    print_log(roc);

    return NORMAL_OP;
}