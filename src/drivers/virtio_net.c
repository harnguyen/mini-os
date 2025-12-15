/**
 * MiniOS - Virtio Network Driver
 * 
 * Driver for QEMU's virtio-net virtual network card.
 * Uses legacy (transitional) virtio interface for simplicity.
 */

#include "types.h"
#include "pci.h"
#include "ports.h"
#include "string.h"
#include "heap.h"

/* Virtio PCI vendor/device IDs */
#define VIRTIO_VENDOR_ID        0x1AF4
#define VIRTIO_NET_DEVICE_ID    0x1000  /* Legacy network device */

/* Virtio PCI configuration offsets (legacy) */
#define VIRTIO_PCI_HOST_FEATURES    0x00
#define VIRTIO_PCI_GUEST_FEATURES   0x04
#define VIRTIO_PCI_QUEUE_PFN        0x08
#define VIRTIO_PCI_QUEUE_SIZE       0x0C
#define VIRTIO_PCI_QUEUE_SEL        0x0E
#define VIRTIO_PCI_QUEUE_NOTIFY     0x10
#define VIRTIO_PCI_STATUS           0x12
#define VIRTIO_PCI_ISR              0x13
#define VIRTIO_PCI_CONFIG           0x14

/* Virtio status bits */
#define VIRTIO_STATUS_ACKNOWLEDGE   0x01
#define VIRTIO_STATUS_DRIVER        0x02
#define VIRTIO_STATUS_DRIVER_OK     0x04
#define VIRTIO_STATUS_FEATURES_OK   0x08
#define VIRTIO_STATUS_FAILED        0x80

/* Virtio ring descriptor flags */
#define VIRTQ_DESC_F_NEXT       0x01
#define VIRTQ_DESC_F_WRITE      0x02

/* Virtio queue sizes */
#define VIRTQ_RX_SIZE   16
#define VIRTQ_TX_SIZE   16

/* Virtio ring descriptor */
typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} PACKED virtq_desc_t;

/* Virtio available ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTQ_RX_SIZE];
    uint16_t used_event;
} PACKED virtq_avail_t;

/* Virtio used ring element */
typedef struct {
    uint32_t id;
    uint32_t len;
} PACKED virtq_used_elem_t;

/* Virtio used ring */
typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[VIRTQ_RX_SIZE];
    uint16_t avail_event;
} PACKED virtq_used_t;

/* Virtio queue */
typedef struct {
    virtq_desc_t* desc;
    virtq_avail_t* avail;
    virtq_used_t* used;
    uint16_t size;
    uint16_t last_used_idx;
    uint8_t** buffers;
} virtq_t;

/* Virtio net header */
typedef struct {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} PACKED virtio_net_hdr_t;

/* Driver state */
static int virtio_initialized = 0;
static uint16_t io_base = 0;
static uint8_t mac_addr[6];
static virtq_t rx_queue;
static virtq_t tx_queue;

/* Buffer size for network packets */
#define NET_BUFFER_SIZE 2048

/**
 * Allocate and initialize a virtqueue
 */
static int virtq_init(virtq_t* vq, uint16_t size) {
    /* Calculate sizes */
    size_t desc_size = size * sizeof(virtq_desc_t);
    size_t avail_size = sizeof(uint16_t) * 3 + sizeof(uint16_t) * size;
    size_t used_size = sizeof(uint16_t) * 3 + sizeof(virtq_used_elem_t) * size;
    
    /* Allocate memory (needs to be page-aligned for virtio) */
    size_t total_size = ALIGN_UP(desc_size + avail_size, PAGE_SIZE) + 
                        ALIGN_UP(used_size, PAGE_SIZE);
    
    uint8_t* mem = (uint8_t*)kcalloc(1, total_size + PAGE_SIZE);
    if (!mem) return -1;
    
    /* Align to page boundary */
    mem = (uint8_t*)ALIGN_UP((uintptr_t)mem, PAGE_SIZE);
    
    vq->desc = (virtq_desc_t*)mem;
    vq->avail = (virtq_avail_t*)(mem + desc_size);
    vq->used = (virtq_used_t*)ALIGN_UP((uintptr_t)(mem + desc_size + avail_size), PAGE_SIZE);
    vq->size = size;
    vq->last_used_idx = 0;
    
    /* Allocate buffer pointers */
    vq->buffers = (uint8_t**)kcalloc(size, sizeof(uint8_t*));
    if (!vq->buffers) return -1;
    
    /* Allocate individual buffers */
    for (int i = 0; i < size; i++) {
        vq->buffers[i] = (uint8_t*)kmalloc(NET_BUFFER_SIZE);
        if (!vq->buffers[i]) return -1;
    }
    
    return 0;
}

/**
 * Set up a virtqueue in the device
 */
static void virtq_setup(int queue_idx, virtq_t* vq) {
    /* Select queue */
    outw(io_base + VIRTIO_PCI_QUEUE_SEL, queue_idx);
    
    /* Get queue size */
    uint16_t size = inw(io_base + VIRTIO_PCI_QUEUE_SIZE);
    if (size == 0 || size > 256) {
        size = 16;  /* Use smaller default */
    }
    
    /* Initialize queue */
    if (virtq_init(vq, size) < 0) {
        return;
    }
    
    /* Tell device about queue location (page frame number) */
    uint32_t pfn = (uint32_t)((uintptr_t)vq->desc / PAGE_SIZE);
    outl(io_base + VIRTIO_PCI_QUEUE_PFN, pfn);
}

/**
 * Add a buffer to the RX queue
 */
static void virtq_add_rx_buffer(virtq_t* vq, int idx) {
    /* Set up descriptor */
    vq->desc[idx].addr = (uint64_t)(uintptr_t)vq->buffers[idx];
    vq->desc[idx].len = NET_BUFFER_SIZE;
    vq->desc[idx].flags = VIRTQ_DESC_F_WRITE;  /* Device writes to this buffer */
    vq->desc[idx].next = 0;
    
    /* Add to available ring */
    uint16_t avail_idx = vq->avail->idx;
    vq->avail->ring[avail_idx % vq->size] = idx;
    
    /* Memory barrier */
    __asm__ volatile("" ::: "memory");
    
    vq->avail->idx = avail_idx + 1;
}

/**
 * Initialize the virtio-net driver
 */
int virtio_net_init(void) {
    pci_device_t dev;
    
    /* Find virtio network device */
    if (!pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_NET_DEVICE_ID, &dev)) {
        return -1;  /* Not found */
    }
    
    /* Get I/O base from BAR0 */
    io_base = dev.bar[0] & 0xFFFC;  /* Remove I/O space bit */
    
    /* Enable bus mastering */
    pci_enable_bus_master(&dev);
    
    /* Reset device */
    outb(io_base + VIRTIO_PCI_STATUS, 0);
    
    /* Acknowledge device */
    outb(io_base + VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    
    /* We're a driver */
    outb(io_base + VIRTIO_PCI_STATUS, 
         VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    
    /* Read features (we don't need any special features) */
    uint32_t features = inl(io_base + VIRTIO_PCI_HOST_FEATURES);
    (void)features;
    
    /* Accept no special features */
    outl(io_base + VIRTIO_PCI_GUEST_FEATURES, 0);
    
    /* Set up virtqueues (0 = RX, 1 = TX) */
    virtq_setup(0, &rx_queue);
    virtq_setup(1, &tx_queue);
    
    /* Add buffers to RX queue */
    for (int i = 0; i < rx_queue.size; i++) {
        virtq_add_rx_buffer(&rx_queue, i);
    }
    
    /* Notify device about RX queue */
    outw(io_base + VIRTIO_PCI_QUEUE_NOTIFY, 0);
    
    /* Read MAC address from config space */
    for (int i = 0; i < 6; i++) {
        mac_addr[i] = inb(io_base + VIRTIO_PCI_CONFIG + i);
    }
    
    /* Driver ready */
    outb(io_base + VIRTIO_PCI_STATUS,
         VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);
    
    virtio_initialized = 1;
    return 0;
}

/**
 * Check if initialized
 */
int virtio_net_is_initialized(void) {
    return virtio_initialized;
}

/**
 * Get MAC address
 */
void virtio_net_get_mac(uint8_t mac[6]) {
    memcpy(mac, mac_addr, 6);
}

/**
 * Send a packet
 */
int virtio_net_send(const void* data, uint16_t len) {
    if (!virtio_initialized || len > NET_BUFFER_SIZE - sizeof(virtio_net_hdr_t)) {
        return -1;
    }
    
    /* Find a free TX buffer (simple: use index 0) */
    static int tx_idx = 0;
    uint8_t* buffer = tx_queue.buffers[tx_idx];
    
    /* Prepare virtio header */
    virtio_net_hdr_t* hdr = (virtio_net_hdr_t*)buffer;
    memset(hdr, 0, sizeof(*hdr));
    
    /* Copy packet data */
    memcpy(buffer + sizeof(virtio_net_hdr_t), data, len);
    
    /* Set up descriptor */
    tx_queue.desc[tx_idx].addr = (uint64_t)(uintptr_t)buffer;
    tx_queue.desc[tx_idx].len = sizeof(virtio_net_hdr_t) + len;
    tx_queue.desc[tx_idx].flags = 0;  /* Device reads from this buffer */
    tx_queue.desc[tx_idx].next = 0;
    
    /* Add to available ring */
    uint16_t avail_idx = tx_queue.avail->idx;
    tx_queue.avail->ring[avail_idx % tx_queue.size] = tx_idx;
    __asm__ volatile("" ::: "memory");
    tx_queue.avail->idx = avail_idx + 1;
    
    /* Notify device */
    outw(io_base + VIRTIO_PCI_QUEUE_NOTIFY, 1);
    
    tx_idx = (tx_idx + 1) % tx_queue.size;
    return 0;
}

/**
 * Receive a packet (non-blocking)
 */
int virtio_net_receive(void* buffer, uint16_t max_len) {
    if (!virtio_initialized) {
        return -1;
    }
    
    /* Check if there are used buffers */
    if (rx_queue.last_used_idx == rx_queue.used->idx) {
        return 0;  /* No packets */
    }
    
    /* Get used buffer */
    uint16_t used_idx = rx_queue.last_used_idx % rx_queue.size;
    uint32_t desc_idx = rx_queue.used->ring[used_idx].id;
    uint32_t len = rx_queue.used->ring[used_idx].len;
    
    rx_queue.last_used_idx++;
    
    /* Copy data (skip virtio header) */
    if (len > sizeof(virtio_net_hdr_t)) {
        len -= sizeof(virtio_net_hdr_t);
        if (len > max_len) len = max_len;
        memcpy(buffer, rx_queue.buffers[desc_idx] + sizeof(virtio_net_hdr_t), len);
    } else {
        len = 0;
    }
    
    /* Re-add buffer to available ring */
    virtq_add_rx_buffer(&rx_queue, desc_idx);
    outw(io_base + VIRTIO_PCI_QUEUE_NOTIFY, 0);
    
    return len;
}

