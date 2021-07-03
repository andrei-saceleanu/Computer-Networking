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
	int key;
	char value[100];
}__attribute__((packed)) set_command;

typedef struct{
	int key;
}__attribute__((packed)) get_command;

#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

#endif
