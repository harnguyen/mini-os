/**
 * MiniOS - Ethernet Layer
 * 
 * Handles Ethernet frame processing.
 */

#include "types.h"
#include "net.h"
#include "string.h"

/* External virtio driver functions */
extern int virtio_net_send(const void* data, uint16_t len);
extern int virtio_net_receive(void* buffer, uint16_t max_len);
extern void virtio_net_get_mac(uint8_t mac[6]);

/* Our MAC address */
static uint8_t our_mac[6];

/* Broadcast MAC address */
static const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/**
 * Initialize ethernet layer
 */
void eth_init(void) {
    virtio_net_get_mac(our_mac);
}

/**
 * Get our MAC address
 */
void eth_get_mac(uint8_t mac[6]) {
    memcpy(mac, our_mac, 6);
}

/**
 * Send an Ethernet frame
 */
int eth_send(const uint8_t dest[6], uint16_t ethertype, const void* data, uint16_t len) {
    uint8_t frame[ETH_FRAME_MAX];
    eth_header_t* hdr = (eth_header_t*)frame;
    
    if (len > ETH_MTU) {
        return -1;
    }
    
    /* Build Ethernet header */
    memcpy(hdr->dest, dest, 6);
    memcpy(hdr->src, our_mac, 6);
    hdr->ethertype = __builtin_bswap16(ethertype);  /* Convert to network byte order */
    
    /* Copy payload */
    memcpy(frame + sizeof(eth_header_t), data, len);
    
    /* Send frame */
    return virtio_net_send(frame, sizeof(eth_header_t) + len);
}

/**
 * Send a broadcast Ethernet frame
 */
int eth_send_broadcast(uint16_t ethertype, const void* data, uint16_t len) {
    return eth_send(broadcast_mac, ethertype, data, len);
}

/**
 * Receive an Ethernet frame
 */
int eth_receive(eth_header_t* hdr_out, void* data, uint16_t max_len) {
    uint8_t frame[ETH_FRAME_MAX];
    
    int len = virtio_net_receive(frame, sizeof(frame));
    if (len <= (int)sizeof(eth_header_t)) {
        return 0;  /* No packet or too small */
    }
    
    /* Copy header */
    memcpy(hdr_out, frame, sizeof(eth_header_t));
    hdr_out->ethertype = __builtin_bswap16(hdr_out->ethertype);  /* Convert to host byte order */
    
    /* Copy payload */
    int payload_len = len - sizeof(eth_header_t);
    if (payload_len > max_len) payload_len = max_len;
    memcpy(data, frame + sizeof(eth_header_t), payload_len);
    
    return payload_len;
}

/**
 * Check if MAC matches ours or is broadcast
 */
int eth_is_for_us(const uint8_t mac[6]) {
    return memcmp(mac, our_mac, 6) == 0 || 
           memcmp(mac, broadcast_mac, 6) == 0;
}

