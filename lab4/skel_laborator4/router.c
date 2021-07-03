#include "skel.h"

int interfaces[ROUTER_NUM_INTERFACES];
struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route
 for the given dest_ip. Or NULL if there is no matching route.
*/
struct route_table_entry *get_best_route(__u32 dest_ip) {
	
    int i;
	struct route_table_entry *best=NULL;
    for(i=0;i<rtable_size;i++){
		struct route_table_entry entry = rtable[i];
		uint32_t net_address = entry.mask & dest_ip;
		if(net_address == entry.prefix){
			if(!best)
				best=&rtable[i];
			else{
				if(entry.mask>best->mask)
					best=&rtable[i];
			}
		}
	}
    return best;
}


/** BONUS **/
int cmp_route(const void*a,const void *b){
	struct route_table_entry x = *(struct route_table_entry *)a;
	struct route_table_entry y = *(struct route_table_entry *)b;
	return y.mask-x.mask;
}

struct route_table_entry *get_best_route_sorted(__u32 dest_ip) {
	
    int i;
    for(i=0;i<rtable_size;i++){
		struct route_table_entry entry = rtable[i];
		uint32_t net_address = entry.mask & dest_ip;
		if(net_address == entry.prefix){
			return &rtable[i];
		}
	}
    return NULL;
}
/*
 Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
 for the given dest_ip or NULL if there is no matching entry.
*/
struct arp_entry *get_arp_entry(__u32 ip) {
  
	int i;
	for(i=0;i<arp_table_len;i++){
		if(arp_table[i].ip==ip)
			return &arp_table[i];
	}
    return NULL;
}

/** BONUS **/
int my_read_rtable(struct route_table_entry *rtable){
	FILE *f;
	fprintf(stderr, "Parsing route table\n");
	f = fopen("rtable.txt", "r");
	DIE(f == NULL, "Failed to open rtable.txt");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char prefix[50], next_hop[50],mask[50],interface[5];
		sscanf(line, "%s %s %s %s", prefix, next_hop,mask,interface);
		fprintf(stderr, "Prefix: %s Next_hop: %s Mask: %s Interface: %s\n", 
							prefix, next_hop,mask,interface);
		rtable[i].prefix = inet_addr(prefix);
		rtable[i].next_hop = inet_addr(next_hop);
		rtable[i].mask = inet_addr(mask);
		rtable[i].interface = atoi(interface);
	}
	fclose(f);
	fprintf(stderr, "Done parsing route table.\n");
	return i;
}


int main(int argc, char *argv[])
{
	msg m;
	int rc;

	init();
	rtable = malloc(sizeof(struct route_table_entry) * 100);
	arp_table = malloc(sizeof(struct  arp_entry) * 100);
	DIE(rtable == NULL, "memory");
	rtable_size = my_read_rtable(rtable);
	qsort(rtable,rtable_size,sizeof(struct route_table_entry),cmp_route);

	parse_arp_table();
	/* Students will write code here */
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		/* TODO 3: Check the checksum */

		uint16_t backup_checksum = ip_hdr->check;
		ip_hdr->check = 0;
		uint16_t checksum=ip_checksum(ip_hdr,sizeof(struct iphdr));
		if(checksum!=backup_checksum){
			printf("Dropped packet checksum%s\n",m.payload);
			continue;
		}

		/* TODO 4: Check TTL >= 1 */

		if(ip_hdr->ttl<1){
			printf("Dropped packet : ttl exceeded%s\n",m.payload);
			continue;
		}

		/* TODO 5: Find best matching route (using the function you wrote at TODO 1) */

		struct route_table_entry *best=get_best_route_sorted(ip_hdr->daddr);

		/* TODO 6: Update TTL and recalculate the checksum */

		ip_hdr->ttl--;
		ip_hdr->check=0;
		ip_hdr->check=ip_checksum(ip_hdr,sizeof(struct iphdr));

		/* TODO 7: Find matching ARP entry and update Ethernet addresses */

		struct arp_entry * matching_entry=get_arp_entry(best->next_hop);
		memcpy(eth_hdr->ether_dhost,matching_entry->mac,6);
		get_interface_mac(best->interface,eth_hdr->ether_shost);

		/* TODO 8: Forward the pachet to best_route->interface */
		rc=send_packet(best->interface,&m);
		DIE(rc < 0,"send_message");
	}
}
