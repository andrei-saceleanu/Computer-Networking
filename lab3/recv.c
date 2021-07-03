#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
    init(HOST,PORT);
    msg r;
    unsigned int checksum;
    int N = 8;
    for (int i = 0; i < COUNT; ++i) {
        memset(r.payload, 0, MSGSIZE);
        checksum = 0;

        if (recv_message(&r) < 0) {
            perror("Error receiving message");
            return -1;
        }

        printf("[%s] Got msg with payload: %s\n", argv[0], r.payload);

        for (int i = 0; i < r.len; ++i) {
            for (int j = 0; j < N; ++j) {
                checksum ^= (1 << j) & r.payload[i];
            }
        }

        memset(r.payload, 0, MSGSIZE);
        //printf("%d %d",r.parity,checksum);
        if (r.parity != checksum) {
            printf("[%s] Error detected\n", argv[0]);
            sprintf(r.payload, "NACK");

        } else {
            printf("[%s] Message is correct\n", argv[0]);
            sprintf(r.payload, "ACK");
        }

        send_message(&r);
    }
    

    return 0;
}
