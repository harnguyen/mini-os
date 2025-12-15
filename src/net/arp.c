/**
 * MiniOS - ARP Protocol
 * 
 * Address Resolution Protocol for mapping IP to MAC addresses.
 */

#include "types.h"
#include "net.h"
#include "string.h"

/* ARP header */
typedef struct {
    uint16_t htype;     /* Hardware type (1 = Ethernet) */
    uint16_t ptype;     /* Protocol type (0x0800 = IPv4) */
    uint8_t  hlen;      /* Hardware address length (6 for Ethernet) */
    uint8_t  plen;      /* Protocol address length (4 for IPv4) */
    uint16_t oper;      /* Operation (1 = request, 2 = reply) */
    uint8_t  sha[6];    /* Sender hardware address */
    uint32_t spa;       /* Sender protocol address */
    uint8_t  tha[6];    /* Target hardware address */
    uint32_t tpa;       /* Target protocol address */
} PACKED arp_packet_t;

/* ARP operation codes */
#define ARP_REQUEST     1
#define ARP_REPLY       2

/* ARP cache entry */
typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    int valid;
} arp_entry_t;

/* ARP cache */
#define ARP_CACHE_SIZE  16
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

/* External ethernet functions */
extern void eth_get_mac(uint8_t mac[6]);
extern int eth_send(const uint8_t dest[6], uint16_t ethertype, const void* data, uint16_t len);
extern int eth_send_broadcast(uint16_t ethertype, const void* data, uint16_t len);

/**
 * Initialize ARP
 */
void arp_init(void) {
    memset(arp_cache, 0, sizeof(arp_cache));
}

/**
 * Look up IP in ARP cache
 */
int arp_lookup(uint32_t ip, uint8_t mac_out[6]) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac_out, arp_cache[i].mac, 6);
            return 1;
        }
    }
    return 0;
}

/**
 * Add entry to ARP cache
 */
static void arp_cache_add(uint32_t ip, const uint8_t mac[6]) {
    /* Find empty or existing slot */
    int slot = -1;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            slot = i;
            break;
        }
        if (arp_cache[i].ip == ip) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        slot = 0;  /* Overwrite first entry if cache is full */
    }
    
    arp_cache[slot].ip = ip;
    memcpy(arp_cache[slot].mac, mac, 6);
    arp_cache[slot].valid = 1;
}

/**
 * Send an ARP request
 */
int arp_request(uint32_t target_ip) {
    arp_packet_t pkt;
    uint8_t our_mac[6];
    
    eth_get_mac(our_mac);
    
    pkt.htype = __builtin_bswap16(1);       /* Ethernet */
    pkt.ptype = __builtin_bswap16(0x0800);  /* IPv4 */
    pkt.hlen = 6;
    pkt.plen = 4;
    pkt.oper = __builtin_bswap16(ARP_REQUEST);
    
    memcpy(pkt.sha, our_mac, 6);
    pkt.spa = net_get_ip();
    memset(pkt.tha, 0, 6);
    pkt.tpa = target_ip;
    
    return eth_send_broadcast(ETHERTYPE_ARP, &pkt, sizeof(pkt));
}

/**
 * Send an ARP reply
 */
static int arp_reply(const uint8_t dest_mac[6], uint32_t dest_ip) {
    arp_packet_t pkt;
    uint8_t our_mac[6];
    
    eth_get_mac(our_mac);
    
    pkt.htype = __builtin_bswap16(1);       /* Ethernet */
    pkt.ptype = __builtin_bswap16(0x0800);  /* IPv4 */
    pkt.hlen = 6;
    pkt.plen = 4;
    pkt.oper = __builtin_bswap16(ARP_REPLY);
    
    memcpy(pkt.sha, our_mac, 6);
    pkt.spa = net_get_ip();
    memcpy(pkt.tha, dest_mac, 6);
    pkt.tpa = dest_ip;
    
    return eth_send(dest_mac, ETHERTYPE_ARP, &pkt, sizeof(pkt));
}

/**
 * Process an incoming ARP packet
 */
void arp_process(const void* data, uint16_t len) {
    if (len < sizeof(arp_packet_t)) {
        return;
    }
    
    const arp_packet_t* pkt = (const arp_packet_t*)data;
    
    /* Verify it's Ethernet + IPv4 */
    if (__builtin_bswap16(pkt->htype) != 1 || 
        __builtin_bswap16(pkt->ptype) != 0x0800 ||
        pkt->hlen != 6 || pkt->plen != 4) {
        return;
    }
    
    /* Always update ARP cache with sender info */
    arp_cache_add(pkt->spa, pkt->sha);
    
    /* Check if this is for us */
    if (pkt->tpa != net_get_ip()) {
        return;
    }
    
    uint16_t oper = __builtin_bswap16(pkt->oper);
    
    if (oper == ARP_REQUEST) {
        /* Send reply */
        arp_reply(pkt->sha, pkt->spa);
    }
    /* For ARP_REPLY, we already cached the info above */
}

