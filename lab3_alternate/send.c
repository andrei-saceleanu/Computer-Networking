#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char *argv[])
{
	msg t;
	int i;
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	/* printf("[SENDER]: BDP=%d\n", atoi(argv[1])); */
	int BDP = atoi(argv[1]);

	int WINDOW_SIZE = BDP / (sizeof(msg)*8);

	int file_desc, chunk_size;
	int N = 8;
	file_desc = open(argv[2], O_RDONLY);

	for(int i = 0;i < WINDOW_SIZE; i++){
		lseek(file_desc, 0, SEEK_SET);
		memset(t.payload, 0, MSGSIZE-4);
		chunk_size = read(file_desc, t.payload, MSGSIZE-5);

		if (chunk_size < 0) {
			perror("Error reading input file\n");
			return -1;
		} else {
			t.len = chunk_size;
			t.checksum = 0;

			for (int j = 0; j < chunk_size; j++) {
				for (int k = 0; k < N; k++) {
					t.checksum ^= (1 << k) & t.payload[j];
				}
			}

			printf("[SENDER] Sending payload: %s\n", t.payload);
		}
		send_message(&t);
	}

	
	for (i = 0; i < COUNT - WINDOW_SIZE; i++) {
		if (recv_message(&t) < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		} else {
			printf("[SENDER] Received response %s\n", t.payload);
		}

		lseek(file_desc, 0, SEEK_SET);
		memset(t.payload, 0, MSGSIZE-4);
		chunk_size = read(file_desc, t.payload, MSGSIZE-5);

		if (chunk_size < 0) {
			perror("Error reading input file\n");
			return -1;
		} else {
			t.len = chunk_size;
			t.checksum = 0;

			for (int j = 0; j < chunk_size; j++) {
				for (int k = 0; k < N; k++) {
					t.checksum ^= (1 << k) & t.payload[j];
				}
			}

			//printf("%u",t.checksum);
			printf("[SENDER] Sending payload: %s\n", t.payload);
		}
		if (send_message(&t) < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}

	for (int i = 0; i < WINDOW_SIZE; i++) {
		if (recv_message(&t) < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		} else {
			printf("[SENDER] Received response %s\n", t.payload);
		}
	}

	printf("[SENDER] Job done, all sent.\n");
		
	return 0;
}
