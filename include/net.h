/**
 * MiniOS - Network Interface
 */

#ifndef _MINIOS_NET_H
#define _MINIOS_NET_H

#include "types.h"

/* Maximum packet sizes */
#define ETH_MTU         1500
#define ETH_FRAME_MAX   1518

/* Ethernet header */
typedef struct {
    uint8_t  dest[6];
    uint8_t  src[6];
    uint16_t ethertype;
} PACKED eth_header_t;

/* Common ethertypes */
#define ETHERTYPE_IPV4  0x0800
#define ETHERTYPE_ARP   0x0806

/**
 * Initialize the network subsystem
 */
void net_init(void);

/**
 * Send a raw Ethernet packet
 * @param data  Packet data (including Ethernet header)
 * @param len   Length of packet in bytes
 * @return      0 on success, negative on error
 */
int net_send_packet(const void* data, uint16_t len);

/**
 * Receive a packet (non-blocking)
 * @param buffer   Buffer to store received packet
 * @param max_len  Maximum buffer size
 * @return         Packet length on success, 0 if no packet, negative on error
 */
int net_receive_packet(void* buffer, uint16_t max_len);

/**
 * Get the MAC address of the network interface
 */
void net_get_mac(uint8_t mac[6]);

/**
 * Get the configured IP address
 * @return IP address in network byte order
 */
uint32_t net_get_ip(void);

/**
 * Set the IP address
 * @param ip IP address in network byte order
 */
void net_set_ip(uint32_t ip);

/**
 * Check if network is initialized
 */
int net_is_initialized(void);

/**
 * Process incoming network packets (call periodically)
 */
void net_poll(void);

/**
 * Send an ICMP echo request (ping)
 * @param dest_ip  Destination IP address
 * @return         0 on success, negative on error
 */
int net_ping(uint32_t dest_ip);

#endif /* _MINIOS_NET_H */

