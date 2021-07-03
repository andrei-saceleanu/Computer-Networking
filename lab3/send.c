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
	int file_desc, chunk_size;
	int N = 8;
	file_desc = open(argv[1], O_RDONLY);

	for (int i = 0; i < COUNT; ++i) {
		lseek(file_desc, 0, SEEK_SET);
		memset(t.payload, 0, MSGSIZE);
		chunk_size = read(file_desc, t.payload, MSGSIZE);

		if (chunk_size < 0) {
			perror("Error reading input file\n");
			return -1;
		} else {
			t.len = chunk_size;
			t.parity = 0;

			for (int i = 0; i < chunk_size; ++i) {
				for (int j = 0; j < N; ++j) {
					t.parity ^= (1 << j) & t.payload[i];
				}
			}

			//printf("%d",t.parity);
			printf("[%s] Sending payload: %s\n", argv[0], t.payload);

			send_message(&t);
			recv_message(&t);

			printf("[%s] Received response: %s\n", argv[0], t.payload);
		}
	}

	close(file_desc);

	return 0;
}
