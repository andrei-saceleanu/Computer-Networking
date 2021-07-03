#include <cstdlib>     /* exit, atoi, malloc, free */
#include <cstdio>
#include <unistd.h>     /* read, write, close */
#include <cstring>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

void compute_cookie(char *message, char *line) {
    strcat(message, line);
    strcat(message, "; ");
}

char *compute_get_request(const char *host, const char *url, char *query_params,
                            char **cookies, int cookies_count,char *token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    //adaugare antet de host
	sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // adaugare antet de autorizare cu token JWT,daca exista
    if(token != NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }
    // adaugare cookie-uri,daca exista
    if (cookies != NULL) {
    	strcat(message,"Cookie: ");
        for(int i = 0; i < cookies_count-1;i++){
            sprintf(line,"%s",cookies[i]);
            compute_cookie(message,line);
        }
        sprintf(line,"%s",cookies[cookies_count-1]);
    	compute_message(message, line);  
    }

    compute_message(message, "");
    return message;
}

char *compute_post_request(const char *host, const char *url, const char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count,char *token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));
    char *body_data_buffer = (char *)calloc(LINELEN, sizeof(char));

    
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
   
	sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
	sprintf(line, "Content-Type: %s",content_type);
    compute_message(message, line);

    // calculare content-length total
    long int len = 0;
    for(int i=0;i<body_data_fields_count;i++){
        len += strlen(body_data[i]);
        strcat(body_data_buffer,body_data[i]);
        if(i<body_data_fields_count-1){
            strcat(body_data_buffer,"&");
            len++;
        }
    }
    sprintf(line, "Content-Length: %ld", len);
    compute_message(message, line);
  
    // adaugare antet de autorizare cu token JWT,daca exista
    if(token != NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }

    // adaugare cookie-uri,daca exista
    if (cookies != NULL) {
        strcat(message,"Cookie: ");
        for(int i = 0; i < cookies_count-1;i++){
            sprintf(line,"%s",cookies[i]);
            compute_cookie(message,line);
        }
        sprintf(line,"%s",cookies[cookies_count-1]);
        compute_message(message, line);  
    }
 
    compute_message(message, "");
	
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);
    return message;
}

char *compute_delete_request(const char *host, char *url,char *token) {
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // adaugare antet de autorizare cu token JWT,daca exista
    if(token != NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }
    compute_message(message, "");

    return message;
}