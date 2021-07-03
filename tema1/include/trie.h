#pragma once
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>
/* ethheader */
#include <net/ethernet.h>
/* ether_header */
#include <arpa/inet.h>
/* icmphdr */
#include <netinet/ip_icmp.h>
/* arphdr */
#include <net/if_arp.h>
#include <asm/byteorder.h>

//un nod al unui trie
//nodurile frunza vor retine un pointer catre entry-ul corespunzator
//din tabela de rutare
//fiecare nivel al arborelui modeleaza un octet din adresele IP,deci
//adancimea maxima este 4,iar fiecare nod trebuie sa aiba 256 fii,
//(0-255 -> valorile posibile pe un octet unsigned)
typedef struct tr{
	int isLeaf;
	struct route_table_entry *rentry;
	struct tr* children[256];
} trie;

struct route_table_entry {//entry ce modeleaza un element din tabela de rutare
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

struct arp_entry {//entry ce modeleaza un element din tabela ARP
	__u32 ip;
	uint8_t mac[6];
};


//construire nod trie
trie* getNode();

//insereaza informatiile din entry-ul dat ca parametru in trie
void insert_trie(trie *root,struct route_table_entry *entry);

//cauta ruta cea mai specifica in trie pentru ip-ul dat ca parametru
struct route_table_entry* search_trie(trie *root,uint32_t dest_ip);