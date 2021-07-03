#include "trie.h"

trie* getNode(){
	trie *newNode = (trie *)malloc(sizeof(trie));
	newNode->isLeaf = 0;
	int i;
	for(i=0;i<256;i++)
		newNode->children[i] = NULL;
	newNode->rentry = NULL;
	return newNode;
}

//izoleaza al n-lea byte (n = 1..4) dintr-o adresa IP/numar unsigned pe 32 biti
// n = 1 => MSB
uint32_t get_nth_byte(uint32_t x,int n){
	if(n==1){
		return (x&0xff000000)>>24;
	}else if(n==2){
		return (x&0x00ff0000)>>16;
	}else if(n==3){
		return (x&0x0000ff00)>>8;
	}
	return x&0x000000ff;

}

void insert_trie(trie *root,struct route_table_entry *entry){
	int i;
	trie *p = root;//prefixul din entry va fi inserat in trie
	for(i=0;i<4;i++){//se ia fiecare octet din prefix
		//octetul curent va fi index-ul fiului pe care continuam parcurgerea
		//in adancime
		uint32_t index = get_nth_byte(entry->prefix,i+1);
		if(!p->children[index]){//daca nu a fost alocat anterior,aloca acum
			p->children[index] = getNode();
		}
		p = p->children[index];//trecere la urmatorul nivel
	}
	p->isLeaf = 1;
	p->rentry = entry;//finalul prefixului inserat retine adresa catre
	//intregul entry din tabela de rutare
}

struct route_table_entry* search_trie(trie *root,uint32_t dest_ip){
	trie *p = root;
	//pornim de la masca cea mai specifica (255.255.255.255)
	uint32_t mask = 0xffffffff;
	int i,j;
	for(i=0;i<32;i++){
		uint32_t searched_prefix = dest_ip & mask;//identificam prefixul
		p = root;
		for(j=0;j<4;j++){
			uint32_t index = get_nth_byte(searched_prefix,j+1);
			if(!p->children[index])
				break;
			p = p->children[index];
		}
		//daca s-a ajuns pe ultimul nivel si masca din entry corespunde
		//cu masca pe care o consideram la iteratia curenta,am gasit
		//ruta cea mai specifica
		if(j==4 && p->rentry->mask == mask){
			return p->rentry;
		}
		mask = mask<<1;//daca nu s-a gasit inca ruta,facem masca mai generala
		//umplem cu zero-uri de la dreapta la stanga
	}
	//absenta oricarei rute catre dest_ip determina intoarcerea NULL
	return NULL;
}