#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#define BROADCAST 2

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	char notification[100];
	char aux[4];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	int clients[MAX_CLIENTS];
	int clients_size = 0;

	fd_set read_fds;	
	fd_set tmp_fds;		
	int fdmax;			

	if (argc < 2) {
		usage(argv[0]);
	}

	
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					
					clients[clients_size++] = newsockfd;
					sprintf(notification,"Client nou: %d\n",newsockfd);

					char others[100] = "Ceilalti clienti sunt: ";
					for(int j = 0;j < clients_size - 1;j++){
						sprintf(aux,"%d",clients[j]);
						strcat(others,aux);
						strcat(others," ");
						send(clients[j],notification,strlen(notification),0);
					}
					if(clients_size > 1){
						strcat(others,"\n");
						send(newsockfd,others,strlen(others),0);
					}
				} else {

					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(i);
						
						sprintf(notification,"Clientul %d a inchis conexiunea\n",i);
						int index = -1;
						for(int j = 0;j<clients_size;j++){
							if(clients[j]!=i){
								send(clients[j],notification,strlen(notification),0);
							}else{
								index = j;
							}
						}

						FD_CLR(i, &read_fds);
						for(int j = index;j<clients_size-1;j++){
							clients[j] = clients[j+1];
						}
						clients_size --;
					} else {
						int dst = atoi(buffer);
						if(dst != BROADCAST)
							send(dst,buffer+2,strlen(buffer)-2,0);
						else{
							for(int j = 0;j<clients_size;j++){
								if(clients[j]!=i){
									send(clients[j],buffer+2,strlen(buffer)-2,0);
								}
							}
						}
						printf ("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
