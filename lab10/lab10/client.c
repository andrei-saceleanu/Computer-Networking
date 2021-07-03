#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define IP_SERVER "34.118.48.238"
#define PORT_SERVER 8080
#define FLD_CNT 2

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;

        
    // Ex 1.1: GET dummy from main server
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    message = compute_get_request(IP_SERVER,"/api/v1/dummy",NULL,NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 1.2: POST dummy and print response from main server
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    char ** bdata = (char **)malloc(FLD_CNT*sizeof(char *));
    for(int i=0;i<FLD_CNT;i++){
        bdata[i] = (char *)malloc(100*sizeof(char));
    }
    strcpy(bdata[0],"id=1");
    strcpy(bdata[1],"name=ceva");
    message = compute_post_request(IP_SERVER,"/api/v1/dummy","application/x-www-form-urlencoded",bdata,FLD_CNT,NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 2: Login into main server
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    strcpy(bdata[0],"username=student");
    strcpy(bdata[1],"password=student");
    message = compute_post_request(IP_SERVER,"/api/v1/auth/login","application/x-www-form-urlencoded",bdata,FLD_CNT,NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 3: GET weather key from main server
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    char ** cookies = (char **)malloc(sizeof(char *));
    for(int i=0;i<FLD_CNT;i++){
        cookies[i] = (char *)malloc(200*sizeof(char));
    }
    strcpy(cookies[0],"connect.sid=s%3A0Izi2NSU8ppADmSKv1AQ4POuSACyqV1Y.0OJ4qcNKFPCUVDTRMiZFMEo429CxEv%2FTf5i0NmpytkI");
    message = compute_get_request(IP_SERVER,"/api/v1/weather/key",NULL,cookies,1);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 4: GET weather data from OpenWeather API
    sockfd = open_connection("37.139.20.5",80,AF_INET,SOCK_STREAM,0);
    message = compute_get_request("37.139.20.5","/data/2.5/weather","lat=45&lon=30&appid=b912dd495585fbf756dc6d8f415a7649",NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 5: POST weather data for verification to main server
    char *from_json = basic_extract_json_response(response);
    sockfd = open_connection("37.139.20.5",80,AF_INET,SOCK_STREAM,0);
    strcpy(cookies[0],"appid=b912dd495585fbf756dc6d8f415a7649");
    message = compute_post_request("37.139.20.5","/data/2.5/weather/45/30","application/json",&from_json,1,cookies,1);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // Ex 6: Logout from main server
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    message = compute_get_request(IP_SERVER,"/api/v1/auth/logout ",NULL,NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);

    // BONUS: make the main server return "Already logged in!"
    sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
    strcpy(bdata[0],"username=student");
    strcpy(bdata[1],"password=student");
    message = compute_post_request(IP_SERVER,"/api/v1/auth/login","application/x-www-form-urlencoded",bdata,FLD_CNT,NULL,0);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);

    strcpy(cookies[0],"connect.sid=s%3AFE_2BrqhMGpoMzGbrbzQwRZ_oRp5zAIH.C1V%2Fr4WTsuzv5gVvsdPKgfEINDHrnQ%2FmQnItb6myDCA");
    message = compute_post_request(IP_SERVER,"/api/v1/auth/login","application/x-www-form-urlencoded",bdata,FLD_CNT,cookies,1);
    send_to_server(sockfd,message);
    printf("%s",message);
    response = receive_from_server(sockfd);
    printf("%s\n",response);
    close_connection(sockfd);
    
    // free the allocated data at the end!
    free(message);
    for(int i=0;i<FLD_CNT;i++){
        free(cookies[i]);
        free(bdata[i]);
    }
    free(cookies);
    free(bdata);
    return 0;
}
