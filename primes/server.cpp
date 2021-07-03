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

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

unordered_map<string,string> credentials;
unordered_map<string,int> cnt;
unordered_map<int,string> sock_to_user;
unordered_map<string,bool> ok;


int prime(int x){
	for(int i=2;i*i<=x;i++){
		if(x%i==0)
			return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, res;
	char buffer[BUFLEN],response[BUFLEN];

	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	credentials.insert({"cineva","ceva"});
	credentials.insert({"admin","qwerty123"});
	string user,pass;
	

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

					FD_SET(newsockfd, &read_fds);

                    if(newsockfd > fdmax) {
                        //s-a alocat un socket mai mare decat maximul de pana acum
                        fdmax = newsockfd;
                    }

				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, 1, 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						string out = sock_to_user[i];
						ok[out]=false;
						sock_to_user.erase(i);
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						for (int j = fdmax; j >=0; j--) {
                           if(FD_ISSET(j, &read_fds)) {
                                fdmax = j;
                                break;
                            }
                        }
					} else {
						cout<<buffer[0]<<"\n";
						if(buffer[0]=='l'){
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							login_command *l = (login_command *)buffer;
							string user = l->user;
							//printf("%s\n", buffer);
							string pass = l->pass;
						
							//cout<<(credentials.find(user)==credentials.end())<<"\n";
							//printf("%s\n", buffer);
							if(credentials.find(user)==credentials.end()||credentials[user]!=pass){
								memset(response,0,BUFLEN);
								sprintf(response,"Credentiale gresite\n");
								send(i,response,BUFLEN,0);
								string out = sock_to_user[i];
								ok[out]=false;
								sock_to_user.erase(i);
								close(i);
								FD_CLR(i, &read_fds);
								for (int j = fdmax; j >=0; j--) {
		                           if(FD_ISSET(j, &read_fds)) {
		                                fdmax = j;
		                                break;
		                            }
		                        }
							}
							ok[user] = true;
							
							sock_to_user[i]=user;
							cout<<i<<" "<<sock_to_user[i]<<"\n";
						}else if(buffer[0]=='v'){
							if(!ok[sock_to_user[i]]){
								memset(response,0,BUFLEN);
								sprintf(response,"Nu esti logat\n");
								send(i,response,BUFLEN,0);
								continue;
							}
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							verify_command *v = (verify_command *)buffer;
							int nr = v->nr;
							if(prime(nr)){
								memset(response,0,BUFLEN);
								sprintf(response,"Numarul %d este prim\n",nr);
								send(i,response,BUFLEN,0);
							}else{
								memset(response,0,BUFLEN);
								sprintf(response,"Numarul %d nu este prim\n",nr);
								send(i,response,BUFLEN,0);
							}
							cnt[sock_to_user[i]]++;
							//printf("%s\n", buffer);
						}else{
							cout<<i<<" "<<sock_to_user[i]<<"\n";
							if(!ok[sock_to_user[i]]){
								memset(response,0,BUFLEN);
								sprintf(response,"Nu esti logat\n");
								send(i,response,BUFLEN,0);
								continue;
							}
							printf("History\n");
							memset(response,0,BUFLEN);
							sprintf(response,"Numarul %d de verificari\n",cnt[sock_to_user[i]]);
							send(i,response,BUFLEN,0);
						}
						
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
