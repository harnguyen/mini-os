/**
 * MiniOS - PCI Bus Driver
 * 
 * Enumerates devices on the PCI bus using configuration space access.
 */

#include "types.h"
#include "pci.h"
#include "ports.h"

/* PCI configuration ports */
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

/* Maximum devices to track */
#define MAX_PCI_DEVICES     32

/* List of detected PCI devices */
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

/**
 * Create PCI configuration address
 */
static uint32_t pci_address(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (1 << 31) |              /* Enable bit */
           ((uint32_t)bus << 16) |
           ((uint32_t)device << 11) |
           ((uint32_t)func << 8) |
           (offset & 0xFC);         /* Align to dword */
}

/**
 * Read a 32-bit value from PCI configuration space
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, func, offset));
    return inl(PCI_CONFIG_DATA);
}

/**
 * Write a 32-bit value to PCI configuration space
 */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, func, offset));
    outl(PCI_CONFIG_DATA, value);
}

/**
 * Read 16-bit value from PCI configuration space
 */
static uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t value = pci_config_read(bus, device, func, offset);
    return (value >> ((offset & 2) * 8)) & 0xFFFF;
}

/**
 * Read 8-bit value from PCI configuration space
 */
static uint8_t pci_config_read8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t value = pci_config_read(bus, device, func, offset);
    return (value >> ((offset & 3) * 8)) & 0xFF;
}

/**
 * Check if a PCI device exists
 */
static int pci_device_exists(uint8_t bus, uint8_t device, uint8_t func) {
    uint16_t vendor = pci_config_read16(bus, device, func, 0);
    return vendor != 0xFFFF;
}

/**
 * Read device information
 */
static void pci_read_device(uint8_t bus, uint8_t device, uint8_t func, pci_device_t* dev) {
    dev->bus = bus;
    dev->device = device;
    dev->function = func;
    
    dev->vendor_id = pci_config_read16(bus, device, func, 0);
    dev->device_id = pci_config_read16(bus, device, func, 2);
    
    uint32_t class_info = pci_config_read(bus, device, func, 8);
    dev->revision = class_info & 0xFF;
    dev->prog_if = (class_info >> 8) & 0xFF;
    dev->subclass = (class_info >> 16) & 0xFF;
    dev->class_code = (class_info >> 24) & 0xFF;
    
    /* Read BARs */
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_config_read(bus, device, func, 0x10 + (i * 4));
    }
    
    /* Read interrupt line */
    dev->irq_line = pci_config_read8(bus, device, func, 0x3C);
}

/**
 * Enumerate all devices on the PCI bus
 */
static void pci_enumerate(void) {
    pci_device_count = 0;
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t func = 0; func < 8; func++) {
                if (!pci_device_exists(bus, device, func)) {
                    if (func == 0) break;  /* No device, skip other functions */
                    continue;
                }
                
                if (pci_device_count < MAX_PCI_DEVICES) {
                    pci_read_device(bus, device, func, &pci_devices[pci_device_count]);
                    pci_device_count++;
                }
                
                /* Check if multi-function device */
                if (func == 0) {
                    uint8_t header_type = pci_config_read8(bus, device, 0, 0x0E);
                    if (!(header_type & 0x80)) {
                        break;  /* Not multi-function, skip other functions */
                    }
                }
            }
        }
    }
}

/**
 * Initialize PCI bus
 */
void pci_init(void) {
    pci_enumerate();
}

/**
 * Find a device by vendor and device ID
 */
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && 
            pci_devices[i].device_id == device_id) {
            *dev = pci_devices[i];
            return 1;
        }
    }
    return 0;
}

/**
 * Find a device by class and subclass
 */
int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* dev) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code && 
            pci_devices[i].subclass == subclass) {
            *dev = pci_devices[i];
            return 1;
        }
    }
    return 0;
}

/**
 * Enable bus mastering for a device
 */
void pci_enable_bus_master(pci_device_t* dev) {
    uint32_t command = pci_config_read(dev->bus, dev->device, dev->function, 0x04);
    command |= (1 << 2);  /* Set Bus Master Enable bit */
    pci_config_write(dev->bus, dev->device, dev->function, 0x04, command);
}

/**
 * Get number of detected devices
 */
int pci_get_device_count(void) {
    return pci_device_count;
}

