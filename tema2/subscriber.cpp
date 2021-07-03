#include "helpers.h"

using namespace std;

//contruire mesaj ce va fi transmis serverului
//se analizeaza fiecare camp din buffer si seteaza corespondentul din 
//protocolul de mesaj catre server
bool make_msg(to_server_msg_t* message, char* buff) {
    buff[strlen(buff) - 1] = 0;
    char *token = strtok(buff, " ");
    if(!token){
        fprintf(stderr, "Available commands are subscribe <topic> <SF>, unsubscribe <topic> or exit.\n");
        return false;
    }

    if (!strcmp(token, "subscribe")) {
        message->type = 's';
    } else if (!strcmp(token, "unsubscribe")) {
        message->type = 'u';
    } else {
        fprintf(stderr, "Available commands are subscribe <topic> <SF>, unsubscribe <topic> or exit.\n");
        return false;
    }

    token = strtok(NULL, " ");
    if(!token){
        fprintf(stderr, "Available commands are subscribe <topic> <SF>, unsubscribe <topic> or exit.\n");
        return false;
    }
    if(strlen(token) > 50){
        fprintf(stderr, "Topic name too long(maximum limit of 50 characters).\n");
        return false;
    }

    strcpy(message->topic_name, token);

    if (message->type == 's') {
        // daca este o comanda de `subscribe`, se analizeaza si al treilea parametru
        token = strtok(NULL, " ");
        if(!token){
            fprintf(stderr, "Available commands are subscribe <topic> <SF>, unsubscribe <topic> or exit.\n");
            return false;
        }
        if(token[0] != '0' && token[0] != '1'){
            fprintf(stderr, "SF flag must be 0 or 1.\n");
            return false;
        }
        message->sf = token[0] - '0';
    }

    return true;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 4, "Usage: ./subscriber <ID> <IP_SERVER> <PORT_SERVER>.\n");
    DIE(strlen(argv[1]) > 10, "Subscriber ID should be at most 10 characters long.\n");

    int sockfd, bytes_num, flag = 1;
    sockaddr_in serv_addr;
    char buff[1+sizeof(to_client_msg_t)];
    
    to_client_msg_t* recv_msg;
    to_server_msg_t sent_msg;
    fd_set fds, tmp_fds;

    //se creeaza socketul dedicat conexiunii la server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "Unable to open server socket.\n");

    //se completeaza datele despre socketul TCP corespunzator conexiunii la server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    DIE(inet_aton(argv[2], &serv_addr.sin_addr) == 0,
            "Incorrect <IP_SERVER>. Conversion failed.\n");
    DIE(connect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0,
            "Unable to connect to server.\n");
    DIE(send(sockfd, argv[1], strlen(argv[1]) + 1, 0) < 0, "Unable to send ID to server.\n");

    //se dezactiveaza algoritmul lui Nagle pentru conexiunea la server
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    //se seteaza file descriptorii socketilor pentru server si pentru stdin
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    FD_SET(0, &fds);

    while (1) {
        tmp_fds = fds;

        DIE(select(sockfd + 1, &tmp_fds, NULL, NULL, NULL) < 0,
               "Unable to select.\n");

        if (FD_ISSET(0, &tmp_fds)) {
            //mesaj de la stdin
            memset(buff, 0, 1 + sizeof(to_client_msg_t));
            fgets(buff, BUFLEN - 1, stdin);

            if (!strcmp(buff, "exit\n")) {
                break;
            }
            if (make_msg(&sent_msg, buff)) {
                DIE(send(sockfd, (char*) &sent_msg, sizeof(sent_msg), 0) < 0,
                       "Unable to send message to server.\n");
   
                if (sent_msg.type == 's') {
                    printf("Subscribed to topic.\n");
                } else {
                    printf("Unsubscribed from topic.\n");
                }
            }
        }

        if (FD_ISSET(sockfd, &tmp_fds)) {
            //mesaj de la server(originand de la un client udp)
            memset(buff, 0, 1 + sizeof(to_client_msg_t));
            bytes_num = recv(sockfd, buff, sizeof(to_client_msg_t), 0);
            DIE(bytes_num < 0, "Error receiving from server.\n");

            if (bytes_num == 0) {
                //conexiune inchisa
                break;
            }
            //se afiseaza mesajul primit
            recv_msg = (to_client_msg_t*)buff;
            printf("%s:%hu - %s - %s - %s\n", recv_msg->ip,recv_msg->udp_port,recv_msg->topic_name, recv_msg->type, recv_msg->data);
        }
    }

    close(sockfd);

    return  0;
}