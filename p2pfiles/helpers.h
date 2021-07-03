#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


typedef struct{
	char ID[4];
	char ip[16];
	uint16_t port;
}__attribute__((packed)) location;

typedef struct{
	char type;
	char file_name[101];
}__attribute__((packed)) command_to_server;


#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

#endif
