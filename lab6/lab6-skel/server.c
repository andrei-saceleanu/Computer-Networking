/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	int fd,bytes_read;

	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in from_station ;
	char buf[BUFLEN];
	socklen_t socklen;

	/*Deschidere socket*/
	int sockfd = socket(PF_INET,SOCK_DGRAM,0);
	DIE(sockfd == -1,"open socket");	
	
	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	from_station.sin_family = AF_INET;
	from_station.sin_port = htons(atoi(argv[1]));
	from_station.sin_addr.s_addr = INADDR_ANY;
	
	/* Legare proprietati de socket */
	int rs = bind(sockfd,(struct sockaddr *)&from_station,sizeof(struct sockaddr));
	DIE(rs == -1 ,"bind");
	
	/* Deschidere fisier pentru scriere */
	DIE((fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1,"open file");
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/
	memset(buf,0,BUFLEN);
	while(bytes_read = recvfrom(sockfd,buf,BUFLEN,0,(struct sockaddr *)&from_station,&socklen)){
		write(fd,buf,bytes_read);
		memset(buf,0,BUFLEN);
	}


	/*Inchidere socket*/	
	close(sockfd);
	/*Inchidere fisier*/
	close(fd);
	return 0;
}
