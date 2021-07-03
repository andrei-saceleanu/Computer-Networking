#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
    msg r,t;
    init(HOST,PORT);
    int fd, remaining, write_size;

    memset(r.payload, 0, MAX_LEN);
    memset(t.payload, 0, MAX_LEN);

    if (recv_message(&r) < 0){
        perror("Receive message\n");
    }

    printf("[recv] Got msg with payload: %s\n", r.payload);

    strcat(r.payload, "-netsent");
    fd = open(r.payload, O_WRONLY | O_CREAT, 0644);

    if(fd < 0){
        perror("Error opening file\n");
    }
    sprintf(t.payload, "ACK(%s)", r.payload);
    t.len = strlen(t.payload) + 1;
    send_message(&t);

    if (recv_message(&r) < 0){
        perror("Receive message\n");
    }

    printf("[recv] Got msg with payload: %s\n", r.payload);

    sprintf(t.payload, "ACK(%s)", r.payload);
    t.len = strlen(t.payload);
    send_message(&t);

    remaining = atoi(r.payload);
    printf("Input file size: %d \n", remaining);

    while (remaining) {
        if (recv_message(&r) < 0) {
            perror("Receive message\n");
        }

        printf("[recv] Got msg with payload: %s\n", r.payload);

        write_size = write(fd, r.payload, r.len);

        if (write_size < 0){
            perror("Error writing file\n");
        }

        send_message(&r);
        remaining -= r.len;
    }

    close(fd);

    return 0;
}
