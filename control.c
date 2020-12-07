#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "airplane.h"
#include "mapperProtocol.h"

#define NO_OF_CONNS 128 //as defined in /proc/sys/net/core/somaxconn
#define MIN_ARGC 3
#define MAX_ARGC 4
#define AIRPORT_ID_ARG 1
#define AIRPORT_INFO_ARG 2
#define MAPPER_PORT_ARG 3
#define MAX_PORT_NO 65535
#define MIN_PORT_NO 1
#define SERVER_FAIL 10

typedef struct sockaddr SockAddr;
typedef struct addrinfo AddrInfo;

// core components of a control
typedef struct {
    const char* id;
    const char* info;
    const char* mapperPort;
    unsigned short portNo; // current server port
    int sockfd; // server socket file descriptor
    AirplaneList airplaneList;
    sem_t lock;
} Control;

// Used for packing data into thread function
typedef struct ThreadData {
    int connFd;
    Control* control;
} ThreadData;

// program exit codes
typedef enum {
    NORMAL_OPERATION = 0,
    INV_ARGC = 1,
    INV_ARGS = 2,
    INV_PORT = 3,
    CONN_FAILED = 4,
    SERVER_FAILED = 5
} Status;

// Given code, print the relevant status message and return the code.
Status print_status(Status code) {
    char* const status[] = {"",
            "Usage: control2310 id info [mapper]",
            "Invalid char in parameter",
            "Invalid port",
            "Can not connect to map",
            ""};
    fprintf(stderr, "%s\n", status[code]);
    return code;
}

// Find an ephemeral port, initialise addrHints. If getting address info
// fails exit with code SERVER_FAILED, else return addrInfo pointer.
AddrInfo* find_ephemeral_port(AddrInfo** addrInfo, AddrInfo* addrHints) {

    // 0 for an ephemeral port
    int err = getaddrinfo("localhost", 0, addrHints, addrInfo);
    if (err) {
        freeaddrinfo(*addrInfo);
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(print_status(SERVER_FAILED)); //should never happen
    }
    return *addrInfo;
}

// Initialise the control server. Find a port, bind, listen for
// connections & print the port number. If any system calls fail, exit
// with code SERVER_FAILED.
int init_server(Control* control) {

    int err;
    AddrInfo* addrInfo = 0;
    AddrInfo addrHints;
    memset(&addrHints, 0, sizeof(AddrInfo));

    addrHints.ai_family = AF_INET; // Specifies IPv4
    addrHints.ai_socktype = SOCK_STREAM; // Connection based byte stream
    addrHints.ai_flags = AI_PASSIVE;  // Because we want to bind with it

    find_ephemeral_port(&addrInfo, &addrHints);

    // create a socket and bind it to a port. 0 for default protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    err = bind(sockfd, (SockAddr*)addrInfo->ai_addr, sizeof(SockAddr));
    if (err) {
        exit(print_status(SERVER_FAILED)); //should never happen
    }

    // Which port did we get?
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);

    // will look at a socket and ask address info about it
    if (getsockname(sockfd, (struct sockaddr*)&addr, &len)) {
        exit(print_status(SERVER_FAILED)); //should never happen
    }

    // allow up to NO_OF_CONNS connection requests to queue
    if (listen(sockfd, NO_OF_CONNS)) {
        exit(print_status(SERVER_FAILED)); //should never happen
    }

    // ensure correct endianness and print portNo
    control->portNo = ntohs(addr.sin_port);
    fprintf(stdout, "%u\n", control->portNo);
    fflush(stdout);

    return sockfd;
}

// Check if the given arg is valid. If not, exit with code INV_ARGS, else
// return the arg.
const char* check_arg(char* arg) {

    for (int i = 0; i < strlen(arg); ++i) {
        if (arg[i] == '\n' || arg[i] == '\r' || arg[i] == ':') {
            exit(print_status(INV_ARGS));
        }
    }
    return arg;
}

// Check if the given arg is a valid port number. If not, exit with code
// INV_PORT, else return the arg.
const char* validate_port(char* arg) {

    char* end;
    unsigned long portNo = strtoul(arg, &end, 10);
    if (*end != '\0' || portNo > MAX_PORT_NO || portNo < MIN_PORT_NO) {
        exit(print_status(INV_PORT));
    }
    return arg;
}

// Initialise the controller - including input validation.
Control* init_control(int argc, char** argv) {

    if (argc != MIN_ARGC && argc != MAX_ARGC) {
        exit(print_status(INV_ARGC));
    }

    Control* control = malloc(sizeof(Control));

    control->id = check_arg(argv[AIRPORT_ID_ARG]);
    control->info = check_arg(argv[AIRPORT_INFO_ARG]);
    control->airplaneList = init_airplane_list();
    // init semaphore unlocked
    sem_init(&control->lock, 0, 1);

    if (argc == MAX_ARGC) {
        control->mapperPort = validate_port(argv[MAPPER_PORT_ARG]);
    } else {
        control->mapperPort = NULL;
    }
    return control;
}

// Given control, initialise a connection to mapper, register the ID and
// port of this airport and disconnect.
void register_id(Control* control) {

    AddrInfo* ai = 0;
    AddrInfo hints;

    memset(&hints, 0, sizeof(AddrInfo));
    hints.ai_family = AF_INET; // IPv4 for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo("localhost", control->mapperPort, &hints, &ai);
    if (err) {
        freeaddrinfo(ai);
        exit(print_status(CONN_FAILED));
    }

    // returns a socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sockfd, (SockAddr*)ai->ai_addr, sizeof(SockAddr))) {
        exit(print_status(CONN_FAILED));
    }

    // separate streams so we can close them independently
    int sockfdCopy = dup(sockfd);
    FILE* contOut = fdopen(sockfd, "w");
    FILE* contIn = fdopen(sockfdCopy, "r");

    // send add airport command to mapper
    fprintf(contOut, "!%s:%u\n", control->id, control->portNo);
    fflush(contOut);

    //disconnect from mapper server
    fclose(contIn);
    fclose(contOut);
}

// Thread function - unpack the struct stored in arg, read the incoming
// request, and respond to it.
void* handle_conn(void* arg) {

    // unpack the struct pointed to by void*
    ThreadData* threadData = arg;
    Control* control = threadData->control;
    int connFd = threadData->connFd;

    // wrap file descriptors in FILE*
    FILE* ctrlIn = fdopen(connFd, "r");
    FILE* ctrlOut = fdopen(dup(connFd), "w");

    const char* msg = parse_str(ctrlIn, '\n');
    fclose(ctrlIn);

    if (!strcmp(msg, "log")) {
        sem_wait(&control->lock);
        // message is log - send back lexicographic list of visited airplanes
        print_airplane_list(control->airplaneList, ctrlOut);
        sem_post(&control->lock);

    } else {
        // message is an id
        Airplane airplane;
        airplane.id = msg;

        sem_wait(&control->lock);

        // this airplane has visited us
        add_airplane(control->airplaneList, airplane);

        fprintf(ctrlOut, "%s\n", control->info);
        fflush(ctrlOut);

        sem_post(&control->lock);
    }

    fclose(ctrlOut);

    return NULL;
}

// Accept connections from the queue and start up a new thread to handle each
// connection.
void accept_conns(Control* control) {

    pthread_t threadId;
    ThreadData* threadData = malloc(sizeof(ThreadData));

    while(true) {

        // pack the threadData
        threadData->control = control;
        threadData->connFd = accept(control->sockfd, 0, 0);

        if (threadData->connFd >= 0) {
            pthread_create(&threadId, 0, handle_conn, threadData);
        } else {
            exit(SERVER_FAIL);
        }
    }

}

int main(int argc, char** argv) {

    Control* control = init_control(argc, argv);
    control->sockfd = init_server(control);

    if (argc == MAX_ARGC) {
        register_id(control);
    }
    accept_conns(control);

    return NORMAL_OPERATION;
}