/**
 * MiniOS - ICMP Protocol
 * 
 * Internet Control Message Protocol for ping.
 */

#include "types.h"
#include "net.h"
#include "string.h"

/* IP header */
typedef struct {
    uint8_t  version_ihl;   /* Version (4) and IHL (5) */
    uint8_t  tos;           /* Type of service */
    uint16_t total_len;     /* Total length */
    uint16_t id;            /* Identification */
    uint16_t flags_frag;    /* Flags and fragment offset */
    uint8_t  ttl;           /* Time to live */
    uint8_t  protocol;      /* Protocol (1 = ICMP) */
    uint16_t checksum;      /* Header checksum */
    uint32_t src_ip;        /* Source IP */
    uint32_t dest_ip;       /* Destination IP */
} PACKED ip_header_t;

/* ICMP header */
typedef struct {
    uint8_t  type;      /* ICMP type */
    uint8_t  code;      /* ICMP code */
    uint16_t checksum;  /* Checksum */
    uint16_t id;        /* Identifier */
    uint16_t seq;       /* Sequence number */
} PACKED icmp_header_t;

/* ICMP types */
#define ICMP_ECHO_REPLY     0
#define ICMP_ECHO_REQUEST   8

/* IP protocol numbers */
#define IP_PROTO_ICMP       1

/* External functions */
extern void eth_get_mac(uint8_t mac[6]);
extern int eth_send(const uint8_t dest[6], uint16_t ethertype, const void* data, uint16_t len);
extern int arp_lookup(uint32_t ip, uint8_t mac_out[6]);
extern int arp_request(uint32_t target_ip);

/* Sequence number for outgoing pings */
static uint16_t ping_seq = 0;

/**
 * Calculate checksum
 */
static uint16_t checksum(const void* data, size_t len) {
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    if (len == 1) {
        sum += *(const uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/**
 * Send an IP packet
 */
static int ip_send(uint32_t dest_ip, uint8_t protocol, const void* data, uint16_t len) {
    uint8_t packet[ETH_MTU];
    ip_header_t* ip = (ip_header_t*)packet;
    
    if (len > ETH_MTU - sizeof(ip_header_t)) {
        return -1;
    }
    
    /* Build IP header */
    ip->version_ihl = 0x45;  /* IPv4, 5 dwords header length */
    ip->tos = 0;
    ip->total_len = __builtin_bswap16(sizeof(ip_header_t) + len);
    ip->id = __builtin_bswap16(ping_seq);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = net_get_ip();
    ip->dest_ip = dest_ip;
    
    /* Calculate header checksum */
    ip->checksum = checksum(ip, sizeof(ip_header_t));
    
    /* Copy payload */
    memcpy(packet + sizeof(ip_header_t), data, len);
    
    /* Look up MAC address */
    uint8_t dest_mac[6];
    if (!arp_lookup(dest_ip, dest_mac)) {
        /* Need to send ARP request */
        arp_request(dest_ip);
        return -2;  /* ARP in progress */
    }
    
    /* Send via Ethernet */
    return eth_send(dest_mac, ETHERTYPE_IPV4, packet, sizeof(ip_header_t) + len);
}

/**
 * Send ICMP echo request (ping)
 */
int icmp_ping(uint32_t dest_ip) {
    uint8_t packet[64];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = __builtin_bswap16(0x1234);
    icmp->seq = __builtin_bswap16(ping_seq++);
    
    /* Add some padding data */
    for (int i = sizeof(icmp_header_t); i < 64; i++) {
        packet[i] = i;
    }
    
    /* Calculate checksum */
    icmp->checksum = checksum(packet, 64);
    
    return ip_send(dest_ip, IP_PROTO_ICMP, packet, 64);
}

/**
 * Send ICMP echo reply
 */
static int icmp_reply(uint32_t dest_ip, uint16_t id, uint16_t seq, 
                      const void* data, uint16_t data_len) {
    uint8_t packet[ETH_MTU];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    
    if (data_len > ETH_MTU - sizeof(icmp_header_t) - sizeof(ip_header_t)) {
        return -1;
    }
    
    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = id;
    icmp->seq = seq;
    
    /* Copy original data */
    memcpy(packet + sizeof(icmp_header_t), data, data_len);
    
    /* Calculate checksum */
    icmp->checksum = checksum(packet, sizeof(icmp_header_t) + data_len);
    
    return ip_send(dest_ip, IP_PROTO_ICMP, packet, sizeof(icmp_header_t) + data_len);
}

/**
 * Process an incoming IP packet
 */
void ip_process(const void* data, uint16_t len) {
    if (len < sizeof(ip_header_t)) {
        return;
    }
    
    const ip_header_t* ip = (const ip_header_t*)data;
    
    /* Verify IP version */
    if ((ip->version_ihl >> 4) != 4) {
        return;
    }
    
    /* Verify it's for us */
    if (ip->dest_ip != net_get_ip()) {
        return;
    }
    
    /* Get header length and payload */
    int ihl = (ip->version_ihl & 0x0F) * 4;
    const uint8_t* payload = (const uint8_t*)data + ihl;
    uint16_t payload_len = __builtin_bswap16(ip->total_len) - ihl;
    
    if (ip->protocol == IP_PROTO_ICMP && payload_len >= sizeof(icmp_header_t)) {
        const icmp_header_t* icmp = (const icmp_header_t*)payload;
        
        if (icmp->type == ICMP_ECHO_REQUEST) {
            /* Reply to ping */
            icmp_reply(ip->src_ip, icmp->id, icmp->seq, 
                      payload + sizeof(icmp_header_t), 
                      payload_len - sizeof(icmp_header_t));
        }
    }
}

