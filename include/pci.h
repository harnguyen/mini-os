/**
 * MiniOS - PCI Bus Interface
 */

#ifndef _MINIOS_PCI_H
#define _MINIOS_PCI_H

#include "types.h"

/* PCI device structure */
typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint32_t bar[6];        /* Base Address Registers */
    uint8_t  irq_line;
} pci_device_t;

/**
 * Initialize PCI bus enumeration
 */
void pci_init(void);

/**
 * Read a 32-bit value from PCI configuration space
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/**
 * Write a 32-bit value to PCI configuration space
 */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/**
 * Find a PCI device by vendor and device ID
 * @return non-zero if found, device info stored in *dev
 */
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* dev);

/**
 * Find a PCI device by class and subclass
 * @return non-zero if found, device info stored in *dev
 */
int pci_find_class(uint8_t class_code, uint8_t subclass, pci_device_t* dev);

/**
 * Enable bus mastering for a PCI device
 */
void pci_enable_bus_master(pci_device_t* dev);

/**
 * Get the number of detected PCI devices
 */
int pci_get_device_count(void);

#endif /* _MINIOS_PCI_H */

