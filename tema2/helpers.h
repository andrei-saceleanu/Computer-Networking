#ifndef HELPERS_H
#define HELPERS_H

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <climits>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>
#include <utility>
#include <unordered_set>
#include <string>
#include <cmath>

using namespace std;

#define DIE(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, "(%s, %d): ",   \
                    __FILE__, __LINE__);    \
            perror(call_description);       \
            exit(EXIT_FAILURE);             \
        }                                   \
    } while(0)


//mesaj adresat clientilor abonati (de tip tcp)
typedef struct {
    char ip[16];
    uint16_t udp_port;
    char topic_name[51];
    char type[11];
    char data[1501];
} __attribute__((packed)) to_client_msg_t;


//mesaj de abonare trimis de clientii tcp catre server
typedef struct {
    char type;
    bool sf;
    char topic_name[51];
} __attribute__((packed)) to_server_msg_t;


//mesaj primit de server de la publisherii udp
typedef struct {
    char topic_name[50];
    uint8_t type;
    char data[1501];
} __attribute__((packed)) from_udp_msg_t;


//structura ce identifica un client 
//cuprinde un map intre topic si daca flagul sf este setat sau nu
//si un set de buffere grupate dupa topic(acestea vor fi trimise la client
//cand se reconecteaza) 
typedef struct {
    unordered_map<string,bool> topic_sf;
    unordered_map<string,vector<to_client_msg_t>> topic_buffers;
} client_t;

#define BUFLEN 1551


#endif  // HELPERS_H
