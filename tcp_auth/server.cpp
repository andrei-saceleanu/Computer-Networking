#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include "helpers.h"
#define BROADCAST 2

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}
map<uint32_t,string> auth_db;

int authenticate_client(int newsockfd,int sockfd,struct sockaddr_in cli_addr){
	char request[100] = "Introduceti username ul si parola:";
	send(newsockfd,request,strlen(request),0);
	char buff[BUFLEN];
	int bytes_recv = recv(newsockfd,buff,BUFLEN,0);
	buff[bytes_recv-1] = '\0';
	string response = buff;
	uint32_t ip = cli_addr.sin_addr.s_addr;
	if(auth_db[ip]==response)
		return 1;
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, res;
	char buffer[BUFLEN];
	char notification[100];
	char aux[4];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	int clients[MAX_CLIENTS];
	int clients_size = 0;
	struct in_addr inaddr;
	inet_aton("127.0.0.1",&inaddr);
	auth_db[inaddr.s_addr] = "ceva parola";

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
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

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					res = authenticate_client(newsockfd,sockfd,cli_addr);
					if(!res){
						char bye[4] = "bye";
						send(newsockfd,bye,4,0);
						close(newsockfd);
						continue;
					}

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}


					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					//adauga client nou si trimite notificari la cei deja abonati
					clients[clients_size++] = newsockfd;
					sprintf(notification,"[UPDATE] Client nou: %d\n",newsockfd);

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
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(i);
						
						sprintf(notification,"[UPDATE] Clientul %d a inchis conexiunea\n",i);
						int index = -1;
						for(int j = 0;j<clients_size;j++){
							if(clients[j]!=i){
								send(clients[j],notification,strlen(notification),0);
							}else{
								index = j;
							}
						}
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						//se scoate din lista de clienti socketul inchis
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
						printf ("S-a primit de la clientul de pe socketul %d mesajul: %s", i, buffer);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
