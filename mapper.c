#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <semaphore.h>
#include "mapperProtocol.h"
#include "airport.h"

#define ARGC 1
#define SERVER_FAILURE 1
#define NO_OF_CONNS 128
#if (DEBUG | CONST_PORT)
#define PORT "12000" //for debugging on a constant port
#else
#define PORT 0
#endif

typedef struct sockaddr SockAddr;
typedef struct addrinfo AddrInfo;

// core components of a mapper
typedef struct Mapper {
    int sockfd;
    AirportList apList;
    sem_t lock;
} Mapper;

// Used for packing data into thread function
typedef struct ThreadData {
    int connFd;
    Mapper* mapper;
} ThreadData;

// Find an ephemeral port, initialise addrHints. If getting address info
// fails exit with code SERVER_FAILURE, else return addrInfo pointer
AddrInfo* find_ephemeral_port(AddrInfo** addrInfo, AddrInfo* addrHints) {

    // 0 for an ephemeral port
    int err = getaddrinfo("localhost", PORT, addrHints, addrInfo);
    if (err) {
        freeaddrinfo(*addrInfo);
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(SERVER_FAILURE); //should never happen
    }
    return *addrInfo;
}

// Initialise the mapper server & return the socket descriptor. If anything
// fails in the server setup exit with code SERVER_FAILURE.
int init_server(void) {

    int err;
    AddrInfo* addrInfo = 0;
    AddrInfo addrHints;
    memset(&addrHints, 0, sizeof(AddrInfo));

    addrHints.ai_family = AF_INET; // IPv4
    addrHints.ai_socktype = SOCK_STREAM;
    addrHints.ai_flags = AI_PASSIVE;  // Because we want to bind with it

    find_ephemeral_port(&addrInfo, &addrHints);

    // create a socket and bind it to a port. 0 for default protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    err = bind(sockfd, (SockAddr*)addrInfo->ai_addr, sizeof(SockAddr));
    if (err) {
        exit(SERVER_FAILURE); //should never happen
    }

    // Which port did we get?
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);

    // will look at a socket and ask address info about it
    if (getsockname(sockfd, (struct sockaddr*)&ad, &len)) {
        exit(SERVER_FAILURE); //should never happen
    }

    // allow up to NO_OF_CONNS connection requests to queue
    if (listen(sockfd, NO_OF_CONNS)) {
        exit(SERVER_FAILURE); //should never happen
    }

    // ensure correct endianness and print portNo
    unsigned short portNo = ntohs(ad.sin_port);
    fprintf(stdout, "%u\n", portNo);
    fflush(stdout);

    return sockfd;
}

// Search mapper for the requested data as per the contents of msg & respond to
// the request by printing to file.
void handle_port_request(Mapper* mapper, MapperMsg msg, FILE* file) {

    // find requested data
    sem_wait(&mapper->lock);
    Airport* airport = get_airport(mapper->apList, msg.args.id);
    sem_post(&mapper->lock);

    // respond to the port request
    if (airport) {
        fprintf(file, "%s\n", airport->port);
        fflush(file);
    } else {
        fprintf(file, ";\n");
        fflush(file);
    }

}

// Check if the airport id contained within the msg is already in mapper's
// list. If it is, ignore it otherwise add it. Thread-safe
void handle_add_airport(Mapper* mapper, MapperMsg msg) {

    sem_wait(&mapper->lock);
    Airport* existingAirport = get_airport(mapper->apList, msg.args.id);
    sem_post(&mapper->lock);

    //if airport is already in list, ignore command
    if (existingAirport != NULL) {
        return;
    }

    Airport airport;
    airport.id = msg.args.id;
    airport.port = msg.args.port;
    airport.info = NULL;

    // otherwise add airport to the airport list
    sem_wait(&mapper->lock);
    add_airport(mapper->apList, airport);
    sem_post(&mapper->lock);
}

// Given, mapper and file, handle the msg in the relevant way depending on its
// type. If the message suggests the connection is closed then kill the
// current thread with pthread_exit.
void process_message(Mapper* mapper, MapperMsg msg, FILE* file) {

    switch (msg.type) {
        case PORT_REQUEST:
            handle_port_request(mapper, msg, file);
            break;
        case ADD_AIRPORT:
            handle_add_airport(mapper, msg);
            break;
        case INFO_REQUEST:
            sem_wait(&mapper->lock);
            print_airport_list(mapper->apList, file);
            sem_post(&mapper->lock);
            break;
        case CONN_CLOSED:
            pthread_exit(NULL);
    }
}

// Thread function - unpack data pointed to by arg, wrap file descriptors in
// FILE pointers, read and process incoming requests/messages.
void* handle_conn(void* arg) {

    // unpack the struct pointed to by void*
    ThreadData* threadData = arg;
    Mapper* mapper = threadData->mapper;
    int connFd = threadData->connFd;

    // wrap file descriptors in FILE*
    int connFdCopy = dup(connFd);
    FILE* mapperIn = fdopen(connFd, "r");
    FILE* mapperOut = fdopen(connFdCopy, "w");

    // exits internally via pthread_exit
    while (true) {
        MapperMsg msg = read_message(mapperIn);
        process_message(mapper, msg, mapperOut);
    }
}

// Test airport list functionality. Insert, get, remove and print elements in
// the airport list.
void test_airport(Mapper* mapper) {

    Airport airport;

    airport.id = "BNE";
    airport.info = "Quaratined";
    airport.port = "4000";
    add_airport(mapper->apList, airport);

    airport.id = "SYD";
    airport.info = "Yaaay";
    airport.port = "5000";
    add_airport(mapper->apList, airport);

    airport.id = "ADL";
    airport.info = "Naaaaaaah";
    airport.port = "6000";
    add_airport(mapper->apList, airport);

    Airport* testAirport = get_airport(mapper->apList, airport.id);
    fprintf(stderr, "testing airport:\n %s\n %s\n %s\n",
            testAirport->id,
            testAirport->info,
            testAirport->port);
    remove_airport(mapper->apList, airport.id);
    Airport* testAirport2 = get_airport(mapper->apList, airport.id);
    if (testAirport2 != NULL) {
        fprintf(stderr, "failed\n");
    }
}

// Initialise the mapper and return a pointer to it.
Mapper* init_mapper(void) {

    Mapper* mapper = malloc(sizeof(Mapper));

    mapper->sockfd = init_server();
    mapper->apList = init_airport_list();
    sem_init(&(mapper->lock), 0, 1);

    return mapper;
}

//Given mapper, accept connections & create threads to handle the incoming
// request. If there's a problem connecting exit code SERVER_FAILURE.
void accept_conns(Mapper* mapper) {

    pthread_t threadId;
    ThreadData* threadData = malloc(sizeof(ThreadData));

    while(true) {

        // pack the threadData
        threadData->mapper = mapper;
        threadData->connFd = accept(mapper->sockfd, 0, 0);

        if (threadData->connFd >= 0) {
            pthread_create(&threadId, 0, handle_conn, threadData);

        } else {
            exit(SERVER_FAILURE);  //todo check if I should exit here?
        }
    }
}

int main(int argc, char** argv) {

    if (argc != ARGC) {
        exit(1); //todo
    }

    Mapper* mapper = init_mapper();
#if DEBUG
    test_airport(mapper);
#endif
    accept_conns(mapper);

    return 0;
}
