#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, newsockfd,filefd,fd;
	struct sockaddr_in serv_addr,cli_addr;
	char buffer[BUFLEN];
    char user[100],pass[100],nr[100];
    fd_set read_fds;
    fd_set tmp_fds;
    int clientfd;
    int fdmax;
    char type;
    login_command *l;
    verify_command *v;

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
            if(strstr(buffer,"LOGIN")==buffer){
            	memset(user,0,100);
	            memset(pass,0,100);
	            l = (login_command *)malloc(sizeof(login_command));
	            p =  strtok(buffer," ");
	            p = strtok(NULL," ");
	            strcpy(user,p);
	            if(user[strlen(user)-1]=='\n'){
	                user[strlen(user)-1]='\0';
	            }
	            strcpy(l->user,user);
	            p = strtok(NULL," ");
	            strcpy(pass,p);
	            if(pass[strlen(pass)-1]=='\n'){
	                pass[strlen(pass)-1]='\0';
	            }
	            strcpy(l->pass,pass);
	            type='l';
	            DIE(send(sockfd,&type,1,0)<0,"Send\n");
	            DIE(send(sockfd,l,sizeof(login_command),0)<0,"Send\n");
	            
            }
            if(strstr(buffer,"VERIFY")==buffer){
            	p =  strtok(buffer," ");
	            p = strtok(NULL," ");
	            strcpy(nr,p);
	            if(nr[strlen(nr)-1]=='\n'){
	                nr[strlen(nr)-1]='\0';
	            }
	            v = (verify_command *)malloc(sizeof(verify_command));
	            type='v';
	            v->nr = atoi(nr);
	            DIE(send(sockfd,&type,1,0)<0,"Send\n");
	            DIE(send(sockfd,v,sizeof(nr),0)<0,"Send\n");
            }
            if(strstr(buffer,"HISTORY")==buffer){
            	type='h';
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
            printf("%s", buffer);
        }
    }


	close(sockfd);

	return 0;
}
