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
#include <cmath>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

unordered_map<int,string> m;
unordered_map<int,int> cnt_ops;
unordered_map<int,int> cnt_bytes;
int server_ops,server_bytes;

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, res;
	int filefd;
	char buffer[BUFLEN],response[BUFLEN],filename[BUFLEN];
	m.insert({6,"Numar secret"});
	char key[100],value[100];
	char type;
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	char *p;
	

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
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if(i == 0){
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					if(strstr(buffer,"LOAD")==buffer){
						memset(filename,0,BUFLEN);
						char *p = strtok(buffer," ");
						p = strtok(NULL," ");
						strcpy(filename,p);
						if(filename[strlen(filename)-1]=='\n'){
							filename[strlen(filename)-1]='\0';
						}
						char cbuff;
						filefd = open(filename,O_RDONLY);
						DIE(filefd < 0,"Open file");
						int first_field = 1;
						string s = "";
						int key;
						string value;
						while(n = read(filefd,&cbuff,1)){
							if(cbuff == ' '){
								if(first_field==1){
									key = stoi(s);
									first_field = 0;
									s="";
								}else{
									s+=cbuff;
								}
							}else if(cbuff=='\n'){
								value = s;
								m.insert({key,value});
								s="";
								first_field = 1;
							}else{
								s+=cbuff;
							}
						}
						close(filefd);
					}
					if(strstr(buffer,"SET")==buffer){
						memset(key,0,100);
						memset(value,0,100);
						set_command *sc = (set_command *)malloc(sizeof(set_command));
						p =  strtok(buffer," ");
						p = strtok(NULL," ");
						strcpy(key,p);
						sc->key = atoi(key);
						p = strtok(NULL," ");
						strcpy(value,p);
						if(value[strlen(value)-1]=='\n'){
							value[strlen(value)-1]='\0';
						}
						strcpy(sc->value,value);
						type='s';
						string val = sc->value;
						int k = sc->key;
						if(m.find(k)!=m.end()){
							m[k]=val;
						}else{
							m.insert({k,val});
						}
						cout<<"Setare cu succes\n";
						server_ops++;
						server_bytes+=strlen(sc->value);
					
					}
					if(strstr(buffer,"GET")==buffer){
						p =  strtok(buffer," ");
						p = strtok(NULL," ");
						memset(key,0,100);
						strcpy(key,p);
						
						get_command *gc = (get_command *)malloc(sizeof(get_command));
						type = 'g';
						gc->key = atoi(key);
						if(m.find(gc->key)==m.end()){
							cout<<"Cheie inexistenta\n";
						}else{
							cout<<m[gc->key]<<"\n";
						}
						server_ops++;
					}
					if(strstr(buffer,"STAT")==buffer){
						printf("Numarul de operatii realizate:%d\nNumarul de bytes folositi:%d\n",server_ops,server_bytes);
					}
					
				} else if (i == sockfd) {
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
						if(buffer[0]=='s'){
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							DIE(n<0,"Receive from client\n");
							set_command *sc = (set_command *)buffer;
							string val = sc->value;
							int key = sc->key;
							if(m.find(key)!=m.end()){
								m[key]=val;
							}else{
								m.insert({key,val});
							}
							memset(response,0,BUFLEN);
							sprintf(response,"Operatie realizata cu succes:SET %d %s\n",sc->key,sc->value);
							DIE(send(i,response,BUFLEN,0)<0,"Send");
							cnt_ops[i]++;
							cnt_bytes[i]+=val.length();
							
						}else if(buffer[0]=='g'){
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							DIE(n<0,"Receive from client\n");
							get_command *gc = (get_command *)buffer;
							if(m.find(gc->key)==m.end()){
								memset(response,0,BUFLEN);
								sprintf(response,"Cheie inexistenta\n");
								send(i,response,BUFLEN,0);
								continue;
							}
							string val = m[gc->key];
							char * value = new char[val.length()];
							strcpy(value,val.c_str());
							send(i,value,val.length(),0);
							cnt_ops[i]++;
						}else{
							printf("Sending current status to client at socket %d\n",i);
							memset(response,0,BUFLEN);
							sprintf(response,"Numarul de operatii realizate:%d\nNumarul de bytes folositi:%d\n",cnt_ops[i],cnt_bytes[i]);
							DIE(send(i,response,BUFLEN,0)<0,"Send");
						}
						
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
