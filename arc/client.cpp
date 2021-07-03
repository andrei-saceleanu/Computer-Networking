#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret ,fd;
	struct sockaddr_in serv_addr,cli_addr;
	char buffer[BUFLEN];
    char key[100],value[100];
    fd_set read_fds;
    fd_set tmp_fds;
    int clientfd;
    int fdmax;
    set_command *sc;
    get_command *gc;
    char type;
 
    

    FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);

	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    FD_SET(0, &read_fds);
    
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(0, &tmp_fds)) {  // citeste de la tastatura
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            char *p;
            if(strstr(buffer,"SET")==buffer){
            	memset(key,0,100);
	            memset(value,0,100);
	            sc = (set_command *)malloc(sizeof(set_command));
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
	            DIE(send(sockfd,&type,1,0)<0,"Send\n");
	            DIE(send(sockfd,sc,sizeof(set_command),0)<0,"Send\n");
	            
            }
            if(strstr(buffer,"GET")==buffer){
            	p =  strtok(buffer," ");
	            p = strtok(NULL," ");
	            memset(key,0,100);
	            strcpy(key,p);
	            
	            gc = (get_command *)malloc(sizeof(get_command));
	            type = 'g';
	            gc->key = atoi(key);
	            DIE(send(sockfd,&type,1,0)<0,"Send\n");
	            DIE(send(sockfd,gc,sizeof(get_command),0)<0,"Send\n");
            }
            if(strstr(buffer,"STAT")==buffer){
            	type='t';
	            DIE(send(sockfd,&type,1,0)<0,"Send\n");
            }
            if(strstr(buffer,"EXIT")==buffer){
            	break;
            }

        } else {
        	memset(buffer, 0, BUFLEN);
        	ret = recv(sockfd, buffer, BUFLEN, 0);
            DIE(ret<0,"Receive\n");
            
            if(!ret){
            	printf("Conexiune inchisa\n");
            	break;
            }
            printf("%s\n", buffer);
        }
    }


	close(sockfd);

	return 0;
}
