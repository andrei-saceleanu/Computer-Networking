#include <queue.h>
#include "skel.h"
#include "trie.h"
#define MAX_ARP_LEN 255
#define MAX_ROUTE_LEN 64265


queue q;
int pkt_id = 10000;
int pkt_seq = 1;
struct arp_entry * arp_table;
struct route_table_entry *rtable;
trie *route;
int rtable_size;
int q_size;
int arp_table_len;

//parsare tabela de rutare prezenta in fisierul denumit filename
//fiecare entry din tabela va fi reprezentat printr-un element
//din vectorul rtable
int read_rtable(struct route_table_entry *rtable,char *filename){
	FILE *f;
	f = fopen(filename, "r");
	route = getNode();//initializare radacina trie
	DIE(f == NULL, "Failed to open route table\n");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char prefix[50], next_hop[50],mask[50],interface[5];
		sscanf(line, "%s %s %s %s", prefix, next_hop,mask,interface);
		// stocare valori in host order
		rtable[i].prefix = ntohl(inet_addr(prefix));
		rtable[i].next_hop = ntohl(inet_addr(next_hop));
		rtable[i].mask = ntohl(inet_addr(mask));
		rtable[i].interface = atoi(interface);
		insert_trie(route,&rtable[i]);//inserare entry in trie
	}
	fclose(f);
	return i;
}

//identificare entry din tabela arp pentru ip-ul dat ca parametru
//tabela arp se retine in network order,deci si parametrul are
//acelasi endianness
struct arp_entry *get_arp_entry(__u32 ip) {
  
	int i;
	for(i=0;i<arp_table_len;i++){
		if(arp_table[i].ip==ip)
			return &arp_table[i];
	}
    return NULL;
}


//gaseste ruta potrivita conform LPM pentru ip-ul dat ca parametru
//vezi sursele trie(trie.h,trie.c) pentru detalii despre stocare
//si cautare
struct route_table_entry *get_best_route(__u32 dest_ip) {
	return search_trie(route,dest_ip);
}


//verifica daca pachetul asociat header-ului dat ca parametru
//este broadcast
int isBroadcast(struct ether_header* eth_hdr){
	int i;
	for(i=0;i<6;i++){
		if(eth_hdr->ether_dhost[i] != 0xff)
			return 0;
	}
	return 1;
}

//verifica daca router-ul curent este destinatia pachetului m
int isRouterTarget(packet m){
	struct ether_header *eth_hdr=(struct ether_header*)(m.payload);
	struct in_addr inaddr;
	inet_aton(get_interface_ip(m.interface),&inaddr);
	if(ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP){
		struct arp_header *arp_hdr = (struct arp_header *)(m.payload + sizeof(struct ether_header));
		if(inaddr.s_addr == arp_hdr->tpa)
			return 1;
	}else if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP){
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		if(inaddr.s_addr == ip_hdr->daddr)
			return 1;
	}
	return 0;
}


//insereaza in tabela arp datele relevante obtinute de la un ARP reply
void updateARP_table(struct arp_header *arp_hdr){
	uint32_t src_ip = arp_hdr->spa;
	arp_table[arp_table_len].ip = src_ip;
	memcpy(arp_table[arp_table_len].mac, arp_hdr->sha,6);
	arp_table_len++; 

}

//construieste si trimite un mesaj de tip ARP_reply asociat
//ARP request-ului m,dat ca parametru,venit pe interfata interface
void sendARP_reply(packet m,int interface){
	struct ether_header *eth_hdr=(struct ether_header*)malloc(sizeof(struct ether_header));
	struct ether_header *packet_eth_hdr=(struct ether_header*)(m.payload);
	struct arp_header *arp_hdr = (struct arp_header *)(m.payload + sizeof(struct ether_header));
	struct in_addr inaddr;
	inet_aton(get_interface_ip(interface),&inaddr);
	uint8_t sha[6];
	get_interface_mac(interface,sha);
	build_ethhdr(eth_hdr,sha,packet_eth_hdr->ether_shost,htons(ETHERTYPE_ARP));
	send_arp(arp_hdr->spa,arp_hdr->tpa,eth_hdr,interface,htons(ARPOP_REPLY));
}


//verifica coada de mesaje
//trimite acele mesaje care au toate informatiile necesare
void checkCacheAndSend(){
	queue temp = queue_create();
	int i,rc;
	int new_size = 0;
	for(i=0;i<q_size;i++){
		packet *current_packet = (packet *)malloc(sizeof(packet));
		memcpy(current_packet,queue_deq(q),sizeof(packet));
		struct ether_header *eth_hdr=(struct ether_header*)(current_packet->payload);
		if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP){
			struct iphdr *ip_hdr = (struct iphdr *)(current_packet->payload + sizeof(struct ether_header));
			struct route_table_entry *best=get_best_route(ntohl(ip_hdr->daddr));
			struct arp_entry *matching_arp_entry = get_arp_entry(htonl(best->next_hop));
			if(matching_arp_entry==NULL){
				//daca tot nu se poate trimite,pune inapoi in coada
				queue_enq(temp,current_packet);
				new_size++;
			}else{//altfel,realizeaza pasii asociati trimiterii
				uint16_t backup_check = ip_hdr->check;
				ip_hdr->check = 0;
				ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
				if(ip_hdr->check!=backup_check){
					continue;
				}
				ip_hdr->ttl--;
				ip_hdr->check = 0;
				ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
				memcpy(eth_hdr->ether_dhost,matching_arp_entry->mac,6);
				get_interface_mac(best->interface,eth_hdr->ether_shost);
				rc=send_packet(best->interface,current_packet);
				DIE(rc < 0,"send_message");
			}
		}
		//similar cu pachetele IP,se face verificarea si pentru cele ARP
		if(ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP){
			struct arp_header *arp_hdr = (struct arp_header *)(current_packet->payload + sizeof(struct ether_header));
			struct route_table_entry *best=get_best_route(ntohl(arp_hdr->tpa));
			struct arp_entry *matching_arp_entry = get_arp_entry(htonl(best->next_hop));
			if(matching_arp_entry==NULL){
				queue_enq(temp,current_packet);
				new_size++;
			}else{
				memcpy(eth_hdr->ether_dhost,matching_arp_entry->mac,6);
				get_interface_mac(best->interface,eth_hdr->ether_shost);
				rc=send_packet(best->interface,current_packet);
				DIE(rc < 0,"send_message");
			}
		}

	}
	//pachetele netrimise se pun inapoi in cache-ul global
	//din coada locala,temporara temp
	for(i=0;i<new_size;i++){
		packet *current_packet = (packet *)malloc(sizeof(packet));
		memcpy(current_packet,queue_deq(temp),sizeof(packet));
		queue_enq(q,current_packet);
	}
	q_size = new_size;

}

//gestionare mesaj primit de tip ARP
//daca este request,se verifica daca una dintre interfete
//are ip-ul cerut.daca se identifica o interfata,se trimite
//un ARP reply
//daca este reply,se updateaza tabela ARP cu datele furnizate
//si se verifica cache-ul de mesaje(coada).se trimit acele mesaje
//care,dupa ce s-a actualizat tabela ARP,au toate informatiile
//necesare la dispozitie.
void processArp(packet m){
	struct arp_header *arp_hdr = (struct arp_header *)(m.payload + sizeof(struct ether_header));
	int i;
	struct in_addr inaddr;
	if(ntohs(arp_hdr->op) == ARPOP_REQUEST){
		uint32_t requested = arp_hdr->tpa;
		for(i=0;i<ROUTER_NUM_INTERFACES;i++){
			inet_aton(get_interface_ip(i),&inaddr);
			if(inaddr.s_addr == requested){
				sendARP_reply(m,i);
				break;
			}
		}
	}

	if(ntohs(arp_hdr->op) == ARPOP_REPLY){
		updateARP_table(arp_hdr);
		checkCacheAndSend();
	}
}


//gestionare pachet IP ICMP destinat router-ului
//se trimite ICMP reply pe interfata de unde a venit
//request-ul
void processIp(packet m){
	struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
	if(ip_hdr->protocol == 1){
		struct ether_header *eth_hdr=(struct ether_header*)(m.payload);
		uint8_t sha[6],dha[6];
		get_interface_mac(m.interface,sha);
		struct in_addr inaddr;
		inet_aton(get_interface_ip(m.interface),&inaddr);
		memcpy(dha,eth_hdr->ether_shost,6);
		send_icmp(ip_hdr->saddr,inaddr.s_addr,sha,dha,0,0,m.interface,pkt_id,pkt_seq++);
	}
}

//gestionare pachet IP ce trebuie dirijat mai departe
//daca nu se cunoaste adresa mac a urmatorului nod catre
//destinatia mesajului,se transmite un ARP request broadcast
//iar mesajul curent este introdus in cache/coada
//altfel,se decrementeaza ttl,se recalculeaza checksum si
//se trimite mesajul pe interfata potrivita
void forwardPacketIp(packet m){
	struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
	struct route_table_entry *best=get_best_route(ntohl(ip_hdr->daddr));
	struct arp_entry *matching_arp_entry = get_arp_entry(htonl(best->next_hop));
	int i,rc;
	if(matching_arp_entry==NULL){
		struct ether_header *req_eth_hdr = (struct ether_header*)malloc(sizeof(struct ether_header));
		uint8_t sha[6];
		get_interface_mac(best->interface,sha);
		struct in_addr inaddr;
		inet_aton(get_interface_ip(best->interface),&inaddr);
		uint8_t broadcast[6];
		for(i=0;i<6;i++){
			broadcast[i]=0xff;
		}
		build_ethhdr(req_eth_hdr,sha,broadcast,htons(ETHERTYPE_ARP));
		send_arp(htonl(best->next_hop),inaddr.s_addr,req_eth_hdr,best->interface,htons(ARPOP_REQUEST));
		packet *cache = (packet*)malloc(sizeof(packet));
		memcpy(cache,&m,sizeof(packet));
		queue_enq(q,cache);
		q_size++;
	}else{
		struct ether_header *eth_hdr=(struct ether_header*)(m.payload);
		uint16_t backup_check = ip_hdr->check;
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
		if(ip_hdr->check!=backup_check){
			return;
		}
		ip_hdr->ttl--;
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
		memcpy(eth_hdr->ether_dhost,matching_arp_entry->mac,6);
		get_interface_mac(best->interface,eth_hdr->ether_shost);
		rc=send_packet(best->interface,&m);
		DIE(rc < 0,"send_message");
	}
}


int main(int argc, char *argv[])
{
	setvbuf(stdout , NULL , _IONBF , 0);
	packet m;
	int rc;
	q = queue_create();
	init(argc - 2, argv + 2);
	rtable = malloc(sizeof(struct route_table_entry) * MAX_ROUTE_LEN);
	arp_table = malloc(sizeof(struct  arp_entry) * MAX_ARP_LEN);
	rtable_size = read_rtable(rtable,argv[1]);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr=(struct ether_header*)(m.payload);
		uint8_t sha[6],dha[6];
		get_interface_mac(m.interface,sha);
		memcpy(dha,eth_hdr->ether_shost,6);
		struct in_addr inaddr;
		inet_aton(get_interface_ip(m.interface),&inaddr);
		//pentru mesajele IP,se verifica ttl sa fie >1
		//sau ca exista ruta catre destinatia dorita
		//in caz ca una dintre conditii nu este indeplinita,pachetul
		//primit se arunca si se trimit mesaje ICMP corespunzatoare
		if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP){
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
			if(ip_hdr->ttl <= 1){
				send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,11,0,m.interface);
				continue;
			}
			struct route_table_entry *best=get_best_route(ntohl(ip_hdr->daddr));
			if(best == NULL){
				send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,3,0,m.interface);
				continue;
			}
		}

		if(isBroadcast(eth_hdr)){//procesare mesaj broadcast ARP
			if(ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP){
				struct arp_header *arp_hdr = (struct arp_header *)(m.payload + sizeof(struct ether_header));
				struct route_table_entry *best=get_best_route(ntohl(arp_hdr->tpa));
				if(best == NULL){
					send_icmp_error(arp_hdr->spa,inaddr.s_addr,sha,dha,3,0,m.interface);
					continue;
				}
				processArp(m);
			}
		}else if(isRouterTarget(m)){//procesare mesaj destinat router-ului
			if(ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP){
				processArp(m);
			}
			if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP){
				struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
				if(ip_hdr->ttl <= 1){
					send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,11,0,m.interface);
					continue;
				}
				struct route_table_entry *best=get_best_route(ntohl(ip_hdr->daddr));
				if(best == NULL){
					send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,3,0,m.interface);
					continue;
				}
				processIp(m);
			}
		}else{//procesare mesaj IP care trebuie dirijat
			if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP){
				struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
				if(ip_hdr->ttl <= 1){
					send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,11,0,m.interface);
					continue;
				}
				struct route_table_entry *best=get_best_route(ntohl(ip_hdr->daddr));
				if(best == NULL){
					send_icmp_error(ip_hdr->saddr,inaddr.s_addr,sha,dha,3,0,m.interface);
					continue;
				}
				forwardPacketIp(m);
			}
		}
	}
}
