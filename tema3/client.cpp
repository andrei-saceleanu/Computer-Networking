#include <cstdio>      /* printf, sprintf */
#include <iostream>
#include <sstream>
#include <cstdlib>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <cstring>     /* memcpy, memset */
#include <string>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

#define PORT_SERVER 8080
#define FLD_CNT 2

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    const char *IP_SERVER = "34.118.48.238";
    char command[100];
    char *session_cookie=NULL,*JWT=NULL; // cookie primit la login si token JWT furnizat de server

    char ** bdata = (char **)malloc(100*sizeof(char *)); // vector de valori din corpul requestului
    for(int i=0;i<FLD_CNT;i++){
        bdata[i] = (char *)malloc(500*sizeof(char));
    }
    char ** cookies = (char **)malloc(100*sizeof(char *)); // vector ce va stoca cookie-urile utilizate
    for(int i=0;i<FLD_CNT;i++){
        cookies[i] = (char *)malloc(500*sizeof(char));
    }

    while(true){
        fgets(command,100,stdin);
        command[strlen(command)-1]='\0';
        if(!strcmp(command,"register")){
            string username,pass;
            cout<<"username=";
            getline(cin,username);
            if(!username.compare("")){
                cout<<"Username shouldn't be empty!\n";
                continue;
            }
            cout<<"password=";
            getline(cin,pass);
            if(!pass.compare("")){
                cout<<"Password shouldn't be empty!\n";
                continue;
            }
            json j;
            j["username"] = username;
            j["password"] = pass;
            string payload = j.dump();
            strcpy(bdata[0],payload.c_str()); // continutul requestului este dat de obiectul json construit
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            message = compute_post_request(IP_SERVER,"/api/v1/tema/auth/register","application/json",bdata,1,NULL,0,NULL);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"The username "<<username<<" is already taken!\n";
            }else{
                cout<<"Register successful!\n";
            }
            close_connection(sockfd);
        }
        if(!strcmp(command,"login")){
            if(session_cookie){
                cout<<"Already logged in!\n";
                continue;
            }
            string username,pass;
            cout<<"username=";
            getline(cin,username);
            cout<<"password=";
            getline(cin,pass);
            json j;
            j["username"] = username;
            j["password"] = pass;
            string payload = j.dump();
            strcpy(bdata[0],payload.c_str());
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            message = compute_post_request(IP_SERVER,"/api/v1/tema/auth/login","application/json",bdata,1,NULL,0,NULL);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"Credentials are not good!\n";
                close_connection(sockfd); 
                continue;
            }
            // se preia cookie-ul din response pentru a dovedi autentificarea la comenzi ulterioare
            char *cookie = strstr(response,"connect.sid=");
            char *p = cookie;
            while(*p!=';'){
                p++;
            }
            int len = p-cookie;
            session_cookie = new char[len+1];
            strncpy(session_cookie,cookie,len);
            cout<<"Your session cookie is: "<<session_cookie<<"\n";
            close_connection(sockfd);   
        }
        if(!strcmp(command,"enter_library")){
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!session_cookie){
                cout<<"You must login first!\n";
                close_connection(sockfd);
                continue;
            }
            if(JWT){
            	cout<<"A token already exists!\n";
            	close_connection(sockfd);
            	continue;
            }
            strcpy(cookies[0],session_cookie);
            message = compute_get_request(IP_SERVER,"/api/v1/tema/library/access",NULL,cookies,1,NULL);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"Not logged in according to server!\n";
                session_cookie = NULL;
                JWT = NULL;
                close_connection(sockfd); 
                continue;
            }
            // se preia din response tokenul JWT pentru a putea dovedi accesul la biblioteca
            // la comenzile ulterioare
            char *body_start = strstr(response,"{");
            char *p = body_start;
            while(*p!='}')
                p++;
            int len = p-body_start+1;
            char *body = new char[len+1];
            strncpy(body,body_start,len);
            body[len] = '\0';
            json r =  json::parse(body);
            string s = r["token"];
            JWT = new char[s.size()+1];
            strcpy(JWT,s.c_str());
            printf("Your token is:%s\n",JWT);
            close_connection(sockfd);
        }
        if(!strcmp(command,"get_books")){
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!JWT){
                cout<<"You dont have access to this library!Try enter_library first!\n";
                close_connection(sockfd);
                continue;
            }
            message = compute_get_request(IP_SERVER,"/api/v1/tema/library/books",NULL,NULL,0,JWT);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            // se preia doar lista de carti din response
            char *body_start = strstr(response,"[");
            char *p = body_start;
            while(*p!=']')
                p++;
            int len = p-body_start+1;
            char *body = new char[len+1];
            strncpy(body,body_start,len);
            body[len] = '\0';
            printf("%s\n",body);
            close_connection(sockfd);
        }
        if(!strcmp(command,"get_book")){
            string id;
            cout<<"id=";
            getline(cin,id);
            string urls = "/api/v1/tema/library/books/"+id;
            char *url = new char[urls.size()+1];
            strcpy(url,urls.c_str());
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!JWT){
                cout<<"You dont have access to this library!Try enter_library first!\n";
                close_connection(sockfd);
                continue;
            }
            message = compute_get_request(IP_SERVER,url,NULL,NULL,0,JWT);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"No book with given id was found!\n";
                close_connection(sockfd); 
                continue;
            }
            // se afiseaza doar cartea cu id-ul cerut
            char *body_start = strstr(response,"[");
            char *p = body_start;
            while(*p!=']')
                p++;
            int len = p-body_start+1;
            char *body = new char[len+1];
            strncpy(body,body_start,len);
            body[len] = '\0';
            printf("%s\n",body);
            close_connection(sockfd); 
        }
        if(!strcmp(command,"add_book")){
            string title,author,genre,publisher;
            int page_count;
            cout<<"title=";
            getline(cin,title);
            if(!title.compare("")){
                cout<<"Title can't be empty!\n";
                close_connection(sockfd);
                continue;
            }
            cout<<"author=";
            getline(cin,author);
            if(!author.compare("")){
                cout<<"Author name can't be empty!\n";
                close_connection(sockfd);
                continue;
            }
            cout<<"genre=";
            getline(cin,genre);
            if(!genre.compare("")){
                cout<<"Genre can't be empty!\n";
                close_connection(sockfd);
                continue;
            }
            cout<<"publisher=";
            getline(cin,publisher);
            if(!publisher.compare("")){
                cout<<"Publisher name can't be empty!\n";
                close_connection(sockfd);
                continue;
            }
            cout<<"page_count=";
            cin>>page_count;
            cin.get();
            if(page_count<=0){
                cout<<"Invalid page count!(either wrong number or string given)\n";
                close_connection(sockfd);
                continue;
            }
            // constructie obiect json si copiere in corpul requestului
            json j;
            j["title"] = title;
            j["author"] = author;
            j["genre"] = genre;
            j["publisher"] = publisher;
            j["page_count"] = page_count;
            string payload = j.dump();
            strcpy(bdata[0],payload.c_str());
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!JWT){
                cout<<"You dont have access to this library!Try enter_library first!\n";
                close_connection(sockfd);
                continue;
            }
            message = compute_post_request(IP_SERVER,"/api/v1/tema/library/books","application/json",bdata,1,NULL,0,JWT);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            cout<<"Book added successfully!\n";
            close_connection(sockfd);  
        }
        if(!strcmp(command,"delete_book")){
            string id;
            cout<<"id=";
            getline(cin,id);
            string urls = "/api/v1/tema/library/books/"+id;
            char *url = new char[urls.size()+1];
            strcpy(url,urls.c_str());
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!JWT){
                cout<<"You dont have access to this library!Try enter_library first!\n";
                close_connection(sockfd);
                continue;
            }
            message = compute_delete_request(IP_SERVER,url,JWT);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"No book was deleted(ID not found)!\n";
                close_connection(sockfd); 
                continue;
            }
            cout<<"Book deleted successfully!\n";
            close_connection(sockfd);  
        }
        if(!strcmp(command,"logout")){
            sockfd = open_connection(IP_SERVER,PORT_SERVER,AF_INET,SOCK_STREAM,0);
            if(!session_cookie){
                cout<<"You must login first!\n";
                close_connection(sockfd);
                continue;
            }
            strcpy(cookies[0],session_cookie);
            message = compute_get_request(IP_SERVER,"/api/v1/tema/auth/logout",NULL,cookies,1,NULL);
            send_to_server(sockfd,message);
            response = receive_from_server(sockfd);
            char *error = strstr(response,"error");
            if(error){
                cout<<"You are not logged in!\n";
                close_connection(sockfd); 
                continue;
            }
            cout<<"Logout successful!\n";
            delete [] session_cookie;
            session_cookie = NULL;
            delete [] JWT;
            JWT = NULL;
            close_connection(sockfd);
        }
        if(!strcmp(command,"exit"))
            break;
    }
    return 0;
}
