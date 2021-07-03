#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(void)
{
	msg r;
	int i;
	unsigned int checksum;
    int N = 8;

	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	for (i = 0; i < COUNT; i++) {
        memset(r.payload, 0, MSGSIZE-4);
        checksum = 0;

        if (recv_message(&r) < 0) {
            perror("Error receiving message");
            return -1;
        }

        printf("[RECEIVER] Got msg with payload: %s\n", r.payload);

        for (int j = 0; j < r.len; j++) {
            for (int k = 0; k < N; k++) {
                checksum ^= (1 << k) & r.payload[j];
            }
        }

		//printf("%u %u",r.checksum,checksum);
        memset(r.payload, 0, MSGSIZE-4);

        
        if (r.checksum != checksum) {
            printf("[RECEIVER] Error detected\n");
            sprintf(r.payload, "NACK");

        } else {
            printf("[RECEIVER] Message is correct\n");
            sprintf(r.payload, "ACK");
        }

        send_message(&r);
	}

	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}
