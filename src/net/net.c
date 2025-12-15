/**
 * MiniOS - Network Interface
 * 
 * Main network subsystem that ties together the layers.
 */

#include "types.h"
#include "net.h"
#include "string.h"

/* External driver/layer functions */
extern int virtio_net_init(void);
extern int virtio_net_is_initialized(void);
extern void virtio_net_get_mac(uint8_t mac[6]);

extern void eth_init(void);
extern void eth_get_mac(uint8_t mac[6]);
extern int eth_receive(eth_header_t* hdr_out, void* data, uint16_t max_len);
extern int eth_is_for_us(const uint8_t mac[6]);

extern void arp_init(void);
extern void arp_process(const void* data, uint16_t len);

extern void ip_process(const void* data, uint16_t len);
extern int icmp_ping(uint32_t dest_ip);

/* Our IP address (default: 10.0.2.15 - QEMU user networking default) */
static uint32_t our_ip = 0x0F02000A;  /* 10.0.2.15 in little-endian */

/* Initialization state */
static int net_inited = 0;

/**
 * Initialize the network subsystem
 */
void net_init(void) {
    /* Initialize virtio-net driver */
    if (virtio_net_init() < 0) {
        return;  /* No network device */
    }
    
    /* Initialize ethernet layer */
    eth_init();
    
    /* Initialize ARP */
    arp_init();
    
    net_inited = 1;
}

/**
 * Check if network is initialized
 */
int net_is_initialized(void) {
    return net_inited && virtio_net_is_initialized();
}

/**
 * Get MAC address
 */
void net_get_mac(uint8_t mac[6]) {
    if (net_inited) {
        eth_get_mac(mac);
    } else {
        memset(mac, 0, 6);
    }
}

/**
 * Get our IP address
 */
uint32_t net_get_ip(void) {
    return our_ip;
}

/**
 * Set our IP address
 */
void net_set_ip(uint32_t ip) {
    our_ip = ip;
}

/**
 * Send a raw Ethernet packet
 */
int net_send_packet(const void* data, uint16_t len) {
    extern int virtio_net_send(const void* data, uint16_t len);
    if (!net_inited) return -1;
    return virtio_net_send(data, len);
}

/**
 * Receive a raw packet
 */
int net_receive_packet(void* buffer, uint16_t max_len) {
    extern int virtio_net_receive(void* buffer, uint16_t max_len);
    if (!net_inited) return -1;
    return virtio_net_receive(buffer, max_len);
}

/**
 * Poll for and process incoming packets
 */
void net_poll(void) {
    if (!net_inited) return;
    
    eth_header_t hdr;
    uint8_t data[ETH_MTU];
    
    /* Receive packet */
    int len = eth_receive(&hdr, data, sizeof(data));
    if (len <= 0) {
        return;
    }
    
    /* Check if it's for us */
    if (!eth_is_for_us(hdr.dest)) {
        return;
    }
    
    /* Process based on ethertype */
    switch (hdr.ethertype) {
        case ETHERTYPE_ARP:
            arp_process(data, len);
            break;
        case ETHERTYPE_IPV4:
            ip_process(data, len);
            break;
        default:
            /* Unknown protocol, ignore */
            break;
    }
}

/**
 * Send a ping (ICMP echo request)
 */
int net_ping(uint32_t dest_ip) {
    if (!net_inited) return -1;
    return icmp_ping(dest_ip);
}

