#include "helpers.h"

using namespace std;


//functie de parsare a unui mesaj primit de la un client udp
//la formatul unui mesaj ce va fi transmis la subscriberi
bool parse_msg(from_udp_msg_t* received, to_client_msg_t* result) {

    if(received->type > 3){
        fprintf(stderr, "Incorrect message type.\n");
        return false;
    }
    strncpy(result->topic_name, received->topic_name, 50);
    result->topic_name[50] = 0;
    long long int int_num;
    double real_num;

    switch (received->type) {
        case 0:
            //s-a primit un INT
            if(received->data[0] > 1){
                fprintf(stderr, "Sign byte must be 0 or 1.\n");
                return false;
            }
            int_num = ntohl(*(uint32_t*)(received->data + 1));

            if (received->data[0]) {
                int_num *= -1;
            }

            sprintf(result->data, "%lld", int_num);
            strcpy(result->type, "INT");
            break;

        case 1:
            //s-a primit un SHORT_REAL
            real_num = ntohs(*(uint16_t*)(received->data));
            real_num /= 100;
            sprintf(result->data, "%.2f", real_num);
            strcpy(result->type, "SHORT_REAL");
            break;

        case 2:
            //s-a primit un FLOAT
            if(received->data[0] > 1){
                fprintf(stderr, "Sign byte must be 0 or 1.\n");
                return false;
            }

            real_num = ntohl(*(uint32_t*)(received->data + 1));
            real_num /= pow(10, received->data[5]);

            if (received->data[0]) {
                real_num *= -1;
            }

            sprintf(result->data, "%lf", real_num);
            strcpy(result->type, "FLOAT");
            break;

        default:
            //s-a primit un STRING
            strcpy(result->type, "STRING");
            strcpy(result->data, received->data);
            break;
    }

    return true;
}


//conversie char array la string
string convertToString(char* a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 2, "Usage: ./server <PORT>\n");

    unordered_map<string,client_t> online,offline;//set de clienti online/offline,indexati dupa id
    unordered_map<string,int> id_to_sock;//mapare id client la socket de comunicare cu acesta
    unordered_map<int,string> sock_to_id;//mapare socket la id client

    char buff[BUFLEN];
    unordered_map<string,client_t>::iterator it;
    int udp_fd,tcp_fd,max_fd,new_sock,bytes_recv,port_num,flag = 1;
    socklen_t socklen = sizeof(sockaddr);
    sockaddr_in udp_addr, tcp_addr, new_tcp;
    
    from_udp_msg_t* udp_msg;
    to_client_msg_t tcp_msg;
    to_server_msg_t* serv_msg;
    client_t new_client;
    fd_set read_fds, tmp_fds;
    bool exit_flag = false;

    //se creeaza socketul UDP
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_fd < 0, "Unable to create UDP socket.\n");

    //se creeaza socketul TCP
    tcp_fd = socket(PF_INET, SOCK_STREAM, 0);
    DIE(tcp_fd < 0, "Unable to create TCP socket.\n");

    port_num = atoi(argv[1]);
    DIE(port_num < 1024, "Incorrect port number.\n");

    //se completeaza informatiile despre socketii udp si tcp
    udp_addr.sin_family = tcp_addr.sin_family = AF_INET;
    udp_addr.sin_port = tcp_addr.sin_port = htons(port_num);
    udp_addr.sin_addr.s_addr = tcp_addr.sin_addr.s_addr = INADDR_ANY;

    DIE(bind(udp_fd, (sockaddr*) &udp_addr, sizeof(sockaddr_in)) < 0,
        "Unable to bind UDP socket.\n");
    DIE(bind(tcp_fd, (sockaddr*) &tcp_addr, sizeof(sockaddr_in)) < 0,
        "Unable to bind TCP socket.\n");

    DIE(listen(tcp_fd, INT_MAX) < 0, "Unable to listen on the TCP socket.\n");

    //se seteaza file descriptorii socketilor creati
    FD_ZERO(&read_fds);
    FD_SET(tcp_fd, &read_fds);
    FD_SET(udp_fd, &read_fds);
    FD_SET(0, &read_fds);
    max_fd = fmax(tcp_fd,udp_fd);

    //serverul ruleaza pana se primeste o comanda de exit
    while(!exit_flag) {
        tmp_fds = read_fds;
        memset(buff, 0, BUFLEN);

        DIE(select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) < 0, "Unable to select.\n");

        for (int i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == 0) {
                    // s-a primit o comanda de la stdin
                    fgets(buff, BUFLEN - 1, stdin);

                    if (!strcmp(buff, "exit\n")) {
                        exit_flag = true;
                        break;
                    } else {
                        fprintf(stderr,"Only available command is exit.\n");
                    }
                } else if (i == udp_fd) {
                    //s-a primit un mesaj de la un client UDP
                    DIE(recvfrom(udp_fd, buff, BUFLEN, 0, (sockaddr*) &udp_addr, &socklen)
                        < 0, "Nothing received from UDP socket.\n");

                    tcp_msg.udp_port = ntohs(udp_addr.sin_port);
                    strcpy(tcp_msg.ip, inet_ntoa(udp_addr.sin_addr));
                    udp_msg = (from_udp_msg_t*)buff;

                    //daca parsarea se realizeaza cu succes
                    if (parse_msg(udp_msg, &tcp_msg)) {
                        //pentru clientii online ,abonati la topicul mesajului,se trimite mesajul nou-venit
                        for(it = online.begin();it!=online.end();it++){
                            if(it->second.topic_sf.find(tcp_msg.topic_name)!=it->second.topic_sf.end()){
                                int fd = id_to_sock[it->first];
                                DIE(send(fd, (char*) &tcp_msg, sizeof(to_client_msg_t), 0) < 0,
                                    "Unable to send message to TCP client");
                            }
                        }
                        //pentru cei offline si abonati,se retine in bufferul corespunzator asociat clientului
                        for(it = offline.begin();it!=offline.end();it++){
                            if(it->second.topic_sf.find(tcp_msg.topic_name)!=it->second.topic_sf.end()){
                                if(it->second.topic_sf[tcp_msg.topic_name]){
                                    to_client_msg_t cached_msg = *(to_client_msg_t*)malloc(sizeof(to_client_msg_t));
                                    memcpy(&cached_msg,&tcp_msg,sizeof(to_client_msg_t));
                                    it->second.topic_buffers[tcp_msg.topic_name].push_back(cached_msg);
                                }
                            }
                        }
                    }
                } else if (i == tcp_fd) {
                    //s-a primit o noua cerere de conexiune TCP
                    new_sock = accept(i, (sockaddr*) &new_tcp, &socklen);
                    DIE(new_sock < 0, "Unable to accept new client.\n");

                    //se dezactiveaza algoritmul lui Nagle pentru conexiunea la clientul TCP
                    setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

                    FD_SET(new_sock, &read_fds);

                    if(new_sock > max_fd) {
                        //s-a alocat un socket mai mare decat maximul de pana acum
                        max_fd = new_sock;
                    }

                    DIE(recv(new_sock, buff, BUFLEN - 1, 0) < 0, "No client ID received.\n");

                    string crt_id = convertToString(buff,strlen(buff));
                    if(online.find(crt_id)!=online.end()){//2 clienti cu acelasi id,nu se accepta noua conexiune
                        cout<<"Client "<<crt_id<<" already connected.\n";
                        FD_CLR(new_sock,&read_fds);//se refac max_fd si fd_set utilizate
                        for (int j = max_fd; j >= 0; j--) {
                           if(FD_ISSET(j, &read_fds)) {
                                max_fd = j;
                                break;
                            }
                        }
                        close(new_sock);
                        continue;
                    }
                    it = offline.find(crt_id);
                    if(it != offline.end()){
                        //clientul conectat a mai avut o sesiune anterioara,se trimit mesajele bufferate care exista
                        id_to_sock.insert({crt_id,new_sock});
                        sock_to_id.insert({new_sock,crt_id});
                        online.insert({crt_id,offline[crt_id]});
                        unordered_map<string,vector<to_client_msg_t>>::iterator it2;
                        for(it2 = it->second.topic_buffers.begin();it2!=it->second.topic_buffers.end();it2++){
                            for(unsigned int j = 0;j<it2->second.size();j++){
                                DIE(send(new_sock, (char*) &(it2->second[j]), sizeof(to_client_msg_t), 0) < 0,
                                    "Unable to send message to TCP client");
                            }
                            it2->second.clear();//se goleste fiecare buffer
                        }
                        offline.erase(it);

                    }else{//clientul este la prima conectare,doar se adauga la setul de clienti online
                        online.insert({crt_id,new_client});
                        id_to_sock.insert({crt_id,new_sock});
                        sock_to_id.insert({new_sock,crt_id});
                    }
                    

                    printf("New client %s connected from %s:%hu.\n", buff,
                           inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
                } else {
                    //se primeste o comanda de la un client TCP
                    bytes_recv = recv(i, buff, BUFLEN - 1, 0);
                    DIE(bytes_recv < 0, "Nothing received from TCP subscriber.\n");

                    if (bytes_recv == 0) {//client deconectat
                        //se refac max_fd si fd_set utilizate
                        //se trece clientul din setul online in cel offline
                        close(i);
                        string id_client_left = sock_to_id[i];
                        cout<<"Client "<<id_client_left<<" disconnected.\n";
                        offline.insert({id_client_left,online[id_client_left]});
                        online.erase(id_client_left);
                        id_to_sock.erase(id_client_left);
                        sock_to_id.erase(i);
                        FD_CLR(i, &read_fds);
                        for (int j = max_fd; j >=0; j--) {
                           if(FD_ISSET(j, &read_fds)) {
                                max_fd = j;
                                break;
                            }
                        }
                    } else {
                        //s-a primit o comanda de subscribe sau unsubscribe
                        serv_msg = (to_server_msg_t*)buff;
                        string id_client = sock_to_id[i];
                        if (serv_msg->type == 'u') {
                            online[id_client].topic_sf.erase(serv_msg->topic_name);
                            online[id_client].topic_buffers[serv_msg->topic_name].clear();
                        } else {
                            online[id_client].topic_sf[serv_msg->topic_name]=serv_msg->sf;
                        }
                    }
                }
            }
        }
    }

    // se inchid toti socketii inca utilizati
    for (int i = 0; i <= max_fd; ++i) {
        if (FD_ISSET(i, &read_fds)) {
            close(i);
        }
    }
    return 0;
}