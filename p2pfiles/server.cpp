#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <unordered_map>
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



unordered_map<string,location> file_to_loc;
unordered_map<string,int> id_to_sock;
unordered_map<int,string> sock_to_id;
unordered_map<string,string> file_to_id;
unordered_map<string,location> id_to_loc;

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, res;
	char buffer[BUFLEN];

	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}
	location loc;
	command_to_server *serv_msg;
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

					FD_SET(newsockfd, &read_fds);

                    if(newsockfd > fdmax) {
                        //s-a alocat un socket mai mare decat maximul de pana acum
                        fdmax = newsockfd;
                    }

                    DIE(recv(newsockfd, buffer, 22, 0) < 0, "No client ID received.\n");
					memcpy(&loc,buffer,sizeof(location));
					sock_to_id.insert({newsockfd,loc.ID});
					id_to_loc.insert({loc.ID,loc});
					id_to_sock.insert({loc.ID,newsockfd});
					printf("Clientul ID:%s disponibil la :%s - %hu\n",loc.ID,loc.ip,ntohs(loc.port));
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
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						string id = sock_to_id[i];
						sock_to_id.erase(i);
						id_to_sock.erase(id);
						for (int j = fdmax; j >=0; j--) {
                           if(FD_ISSET(j, &read_fds)) {
                                fdmax = j;
                                break;
                            }
                        }
					} else {
						serv_msg = (command_to_server *)buffer;
						string id = sock_to_id[i];
						file_to_id.insert({serv_msg->file_name,id});
						if(serv_msg->type=='s'){
							file_to_loc.insert({serv_msg->file_name,id_to_loc[id]});
						}
						if(serv_msg->type=='u'){
							file_to_loc.erase(serv_msg->file_name);
							file_to_id.erase(serv_msg->file_name);
						}
						if(serv_msg->type=='d'){
							location *new_loc = (location*)malloc(sizeof(location));
							memcpy(new_loc,&file_to_loc[serv_msg->file_name],sizeof(location));
							DIE(send(i,(char *)new_loc,sizeof(location),0)<0,"Send location\n");
						}
						
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
