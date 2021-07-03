#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv){
    init(HOST,PORT);
    msg t;
    int fd, size, read_size;

    memset(t.payload, 0, MAX_LEN);

    sprintf(t.payload, argv[1]);
    t.len = strlen(t.payload) + 1;
    send_message(&t);

    if (recv_message(&t) < 0){
        perror("Receive error\n");
    }
    else {
        printf("[send] Got reply with payload: %s\n", t.payload);
    }

    fd = open(argv[1], O_RDONLY);
    if(fd < 0){
        perror("Error opening input file\n");
    }
    size = lseek(fd, 0, SEEK_END);

    sprintf(t.payload, "%d", size);
    t.len = strlen(t.payload) + 1;
    send_message(&t);

    lseek(fd, 0, SEEK_SET);

    if (recv_message(&t) < 0){
        perror("Receive error\n");
    }
    else {
        printf("[send] Got reply with payload: %s\n", t.payload);
    }

    while (read_size = read(fd, t.payload, MAX_LEN )) {
        if (read_size < 0) {
            perror("Error reading input file\n");
        } else {
            t.len = read_size;
            send_message(&t);

            if (recv_message(&t) < 0){
                perror("Receive error\n");
            }
            else {
                printf("[send] Got reply with payload: ACK(%s)\n", t.payload);
            }

            memset(t.payload, 0, MAX_LEN);
        }
    }

    close(fd);

    return 0;
}
