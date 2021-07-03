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
	fprintf(stderr, "Usage: %s ID share_address share_port server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, newsockfd,filefd,fd;
	struct sockaddr_in serv_addr,cli_addr,caddr;
	char buffer[BUFLEN];
    char filename[100],recv_filename[100];
    fd_set read_fds;
    fd_set tmp_fds;
    int clientfd;
    int fdmax;

    FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);

	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    clientfd = socket(AF_INET,SOCK_STREAM,0);
    filefd = socket(AF_INET,SOCK_STREAM,0);
	DIE(sockfd < 0, "socket");

    FD_SET(sockfd, &read_fds);
    FD_SET(clientfd,&read_fds);
    fdmax = sockfd>clientfd?sockfd:clientfd;
    FD_SET(0, &read_fds);
    struct sockaddr_in file_addr;
    file_addr.sin_family = AF_INET;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[5]));
	ret = inet_aton(argv[4], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
    location l;
    command_to_server command;
    strcpy(l.ID,argv[1]);
    strcpy(l.ip,argv[2]);
    unsigned int p = atoi(argv[3]);
    uint16_t port = p;
    l.port = htons(p);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(atoi(argv[3]));
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(clientfd, (struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");
    ret = listen(clientfd, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    DIE(send(sockfd, (char *)&l, sizeof(l), 0) < 0, "Unable to send share details to server.\n");

	while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(0, &tmp_fds)) {  // citeste de la tastatura
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            char *p;
            if (strstr(buffer,"SHARE")==buffer){
                command.type = 's';
            }
            if (strstr(buffer,"UNSHARE")==buffer){
                command.type = 'u';
            }
            if (strstr(buffer,"DOWNLOAD")==buffer){
                command.type = 'd';
            }
            p =  strtok(buffer," ");
            p = strtok(NULL," ");
            memset(filename,0,100);
            strcpy(filename,p);
            if(filename[strlen(filename)-1]=='\n'){
                filename[strlen(filename)-1]='\0';
            }
            strcpy(command.file_name,filename);
            DIE(send(sockfd,(char *)&command,sizeof(command),0)<0,"Send\n");

        } else if(FD_ISSET(sockfd,&tmp_fds)){ //doar daca s-a dat download
            memset(buffer, 0, BUFLEN);
            recv(sockfd, buffer, BUFLEN, 0);
            location *dest = (location *)buffer;
            printf("Pentru %s conecteaza-te la:%s - %hu\n",filename, dest->ip,ntohs(dest->port));
            file_addr.sin_port = dest->port;
            inet_aton(dest->ip,&file_addr.sin_addr);
            ret = connect(filefd, (struct sockaddr*) &file_addr, sizeof(serv_addr));
            DIE(ret < 0, "connect");
            send(filefd,filename,100,0);
            int file = open("received.txt", O_WRONLY | O_CREAT, 0644);
            while(true){
                n = recv(filefd,buffer,BUFLEN,0);
                if(n==0)
                    break;
                write(file,buffer,sizeof(buffer));
            }
            close(file);
        }else{
            socklen_t clilen = sizeof(caddr);
            newsockfd = accept(clientfd, (struct sockaddr *) &caddr, &clilen);
            DIE(newsockfd < 0, "accept");
            recv(newsockfd,recv_filename,100,0);
            fd = open(recv_filename,O_RDONLY);
            while(n = read(fd,buffer,sizeof(buffer))){
                send(newsockfd,buffer,sizeof(buffer),0);
            }
            close(newsockfd);
            close(fd);
        }
	}

	close(sockfd);

	return 0;
}
