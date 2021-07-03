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



unordered_map<string,int> id_to_sock;
unordered_map<int,string> sock_to_id;
unordered_set<string> pending;
unordered_map<int,int> game_pairs;
unordered_set<string>::iterator it;

int check_end(int table[3][3]){
	if(table[0][0]==table[0][1]&&table[0][1]==table[0][2]&&table[0][0]==1)
		return 1;
	if(table[1][0]==table[1][1]&&table[1][1]==table[1][2]&&table[1][0]==1)
		return 1;
	if(table[2][0]==table[2][1]&&table[2][1]==table[2][2]&&table[2][0]==1)
		return 1;
	if(table[0][0]==table[1][0]&&table[1][0]==table[2][0]&&table[0][0]==1)
		return 1;
	if(table[0][1]==table[1][1]&&table[1][1]==table[2][1]&&table[0][1]==1)
		return 1;
	if(table[0][2]==table[1][2]&&table[1][2]==table[2][2]&&table[0][2]==1)
		return 1;
	if(table[0][0]==table[1][1]&&table[1][1]==table[2][2]&&table[0][0]==1)
		return 1;
	if(table[0][2]==table[1][1]&&table[1][1]==table[2][0]&&table[0][2]==1)
		return 1;

	if(table[0][0]==table[0][1]&&table[0][1]==table[0][2]&&table[0][0]==2)
		return 2;
	if(table[1][0]==table[1][1]&&table[1][1]==table[1][2]&&table[1][0]==2)
		return 2;
	if(table[2][0]==table[2][1]&&table[2][1]==table[2][2]&&table[2][0]==2)
		return 2;
	if(table[0][0]==table[1][0]&&table[1][0]==table[2][0]&&table[0][0]==2)
		return 2;
	if(table[0][1]==table[1][1]&&table[1][1]==table[2][1]&&table[0][1]==2)
		return 2;
	if(table[0][2]==table[1][2]&&table[1][2]==table[2][2]&&table[0][2]==2)
		return 2;
	if(table[0][0]==table[1][1]&&table[1][1]==table[2][2]&&table[0][0]==2)
		return 2;
	if(table[0][2]==table[1][1]&&table[1][1]==table[2][0]&&table[0][2]==2)
		return 2;
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			if(table[i][j]==0){
				return 0;
			}
		}
	}
	return -1;
}

game *games;
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, res;
	char buffer[BUFLEN];
	char notification[100],response[100];

	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	command_to_server* serv_msg;

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

	games = (game *)malloc(MAX_CLIENTS/2*sizeof(game));
	int no_games = 0;

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

                    DIE(recv(newsockfd, buffer, sizeof(buffer), 0) < 0, "No client ID received.\n");
                    string id = buffer;
					sock_to_id.insert({newsockfd,id});
					id_to_sock.insert({id,newsockfd});
					if(pending.size()==0){
						pending.insert(id);
					}else{
						it = pending.begin();
						printf("New match between %s and %s\n",*it,id);
						sprintf(notification,"Your opponent is:%s",id);
						send(id_to_sock[*it],notification,100,0);
						sprintf(notification,"Your opponent is:%s",*it);
						send(id_to_sock[id],notification,100,0);
						games[no_games].player1 = id_to_sock[*it];
						games[no_games].player2 = id_to_sock[id];
						for(int j=0;j<3;j++){
							for(int k=0;k<3;k++){
								games[no_games].table[j][k]=0;
							}
						}
						games[no_games].turn = 1;
						no_games++;
						game_pairs.insert({id_to_sock[*it],id_to_sock[id]});
						game_pairs.insert({id_to_sock[id],id_to_sock[*it]});
						pending.erase(it);
					}
					printf("Clientul ID:%s disponibil la :%s - %hu\n",id,inet_ntoa(cli_addr.sin_addr),ntohs(id,cli_addr.sin_port));
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
						if(pending.find(id)!=pending.end()){
							sprintf(response,"The game hasnt started yet.\n");
							DIE(send(i,response,100,0)<0,"Error sending response.\n");
							continue;
						}
						int pair = game_pairs[i];
						int i_first = 0;
						int j;
						int x = serv_msg->i;
						int y = serv_msg->j;
						for(j=0;j<no_games;j++){
							if(games[j].player1==i&&games[j].player2==pair){
								i_first = 1;
								break;
							}
							if(games[j].player2==i&&games[j].player1==pair)
								break;
						}
						if(i_first){
							if(games[j].turn!=1){
								sprintf(response,"Not your turn.\n");
								DIE(send(i,response,100,0)<0,"Error sending response.\n");
								continue;
							}
						}else{
							if(games[j].turn!=2){
								sprintf(response,"Not your turn.\n");
								DIE(send(i,response,100,0)<0,"Error sending response.\n");
								continue;
							}
						}
						if(games[j].table[x][y]!=0){
							sprintf(response,"Position already filled.\n");
							DIE(send(i,response,100,0)<0,"Error sending response.\n");
							continue;
						}
						if(check_end(games[j].table)==-1){
							sprintf(response,"Stalemate\n");
							DIE(send(i,response,100,0)<0,"Error sending response.\n");
							DIE(send(pair,response,100,0)<0,"Error sending response.\n");
							FD_CLR(i,&read_fds);
							FD_CLR(pair,&read_fds);
							close(i);
							close(pair);
							for (int j = fdmax; j >=0; j--) {
	                           if(FD_ISSET(j, &read_fds)) {
	                                fdmax = j;
	                                break;
	                            }
    	                    }
    	                    
						}
						games[j].table[x][y] = games[j].turn;
						games[j].turn = 3- games[j].turn;

						
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
