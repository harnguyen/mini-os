/**
 * MiniOS - ATA/IDE Disk Driver
 * 
 * PIO mode driver for ATA hard drives.
 */

#include "types.h"
#include "ata.h"
#include "ports.h"

/* ATA I/O port base addresses */
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

/* ATA register offsets (from I/O base) */
#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBA_LO      0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HI      0x05
#define ATA_REG_DRIVE       0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07

/* ATA commands */
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_IDENTIFY    0xEC

/* ATA status bits */
#define ATA_SR_BSY          0x80    /* Busy */
#define ATA_SR_DRDY         0x40    /* Drive ready */
#define ATA_SR_DF           0x20    /* Drive fault */
#define ATA_SR_DSC          0x10    /* Drive seek complete */
#define ATA_SR_DRQ          0x08    /* Data request */
#define ATA_SR_CORR         0x04    /* Corrected data */
#define ATA_SR_IDX          0x02    /* Index */
#define ATA_SR_ERR          0x01    /* Error */

/* Drive selection */
#define ATA_DRIVE_MASTER    0xE0
#define ATA_DRIVE_SLAVE     0xF0

/* Current I/O base and control base */
static uint16_t ata_io_base = ATA_PRIMARY_IO;
static uint16_t ata_ctrl_base = ATA_PRIMARY_CTRL;
static int ata_drive_present = 0;

/**
 * Wait for drive to be ready (not busy)
 */
static int ata_wait_ready(void) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ata_io_base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            return 0;
        }
    }
    return -1;  /* Timeout */
}

/**
 * Wait for data request
 */
static int ata_wait_drq(void) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ata_io_base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            return -1;  /* Error */
        }
        if (status & ATA_SR_DRQ) {
            return 0;   /* Data ready */
        }
    }
    return -1;  /* Timeout */
}

/**
 * Software reset the ATA bus
 */
static void ata_soft_reset(void) {
    outb(ata_ctrl_base, 0x04);  /* Set SRST bit */
    io_wait();
    io_wait();
    io_wait();
    io_wait();
    outb(ata_ctrl_base, 0x00);  /* Clear SRST bit */
    io_wait();
}

/**
 * Identify drive
 */
static int ata_identify(void) {
    /* Select master drive */
    outb(ata_io_base + ATA_REG_DRIVE, ATA_DRIVE_MASTER);
    io_wait();
    
    /* Clear sector count and LBA registers */
    outb(ata_io_base + ATA_REG_SECCOUNT, 0);
    outb(ata_io_base + ATA_REG_LBA_LO, 0);
    outb(ata_io_base + ATA_REG_LBA_MID, 0);
    outb(ata_io_base + ATA_REG_LBA_HI, 0);
    
    /* Send IDENTIFY command */
    outb(ata_io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();
    
    /* Check if drive exists */
    uint8_t status = inb(ata_io_base + ATA_REG_STATUS);
    if (status == 0) {
        return 0;  /* No drive */
    }
    
    /* Wait for BSY to clear */
    if (ata_wait_ready() < 0) {
        return 0;  /* Timeout */
    }
    
    /* Check LBA mid and high - if non-zero, not ATA */
    if (inb(ata_io_base + ATA_REG_LBA_MID) != 0 ||
        inb(ata_io_base + ATA_REG_LBA_HI) != 0) {
        return 0;  /* Not ATA (might be ATAPI) */
    }
    
    /* Wait for DRQ or ERR */
    int timeout = 100000;
    while (timeout--) {
        status = inb(ata_io_base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            return 0;  /* Error */
        }
        if (status & ATA_SR_DRQ) {
            break;
        }
    }
    
    if (timeout <= 0) {
        return 0;  /* Timeout */
    }
    
    /* Read identify data (256 words) */
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ata_io_base + ATA_REG_DATA);
    }
    
    /* Identify data received - drive exists */
    (void)identify_data;  /* Not used for now */
    return 1;
}

/**
 * Initialize ATA driver
 */
void ata_init(void) {
    ata_drive_present = 0;
    
    /* Try primary master */
    ata_io_base = ATA_PRIMARY_IO;
    ata_ctrl_base = ATA_PRIMARY_CTRL;
    
    /* Check if controller exists */
    uint8_t status = inb(ata_io_base + ATA_REG_STATUS);
    if (status == 0xFF) {
        /* No controller on primary bus, try secondary */
        ata_io_base = ATA_SECONDARY_IO;
        ata_ctrl_base = ATA_SECONDARY_CTRL;
        status = inb(ata_io_base + ATA_REG_STATUS);
        if (status == 0xFF) {
            return;  /* No ATA controller found */
        }
    }
    
    /* Soft reset */
    ata_soft_reset();
    
    /* Try to identify drive */
    if (ata_identify()) {
        ata_drive_present = 1;
    }
}

/**
 * Check if drive is present
 */
int ata_is_present(void) {
    return ata_drive_present;
}

/**
 * Read sectors from disk (PIO mode)
 */
int ata_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    if (!ata_drive_present) {
        return -1;
    }
    
    if (count == 0) {
        count = 1;  /* Treat 0 as 256 would be confusing, use 1 */
    }
    
    /* Wait for drive to be ready */
    if (ata_wait_ready() < 0) {
        return -1;
    }
    
    /* Select drive with LBA mode */
    outb(ata_io_base + ATA_REG_DRIVE, ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    io_wait();
    
    /* Set sector count and LBA */
    outb(ata_io_base + ATA_REG_SECCOUNT, count);
    outb(ata_io_base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(ata_io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ata_io_base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    /* Send READ command */
    outb(ata_io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    /* Read sectors */
    uint16_t* buf = (uint16_t*)buffer;
    for (int sector = 0; sector < count; sector++) {
        /* Wait for data */
        if (ata_wait_drq() < 0) {
            return -1;
        }
        
        /* Read 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(ata_io_base + ATA_REG_DATA);
        }
    }
    
    return 0;
}

/**
 * Write sectors to disk (PIO mode)
 */
int ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    if (!ata_drive_present) {
        return -1;
    }
    
    if (count == 0) {
        count = 1;
    }
    
    /* Wait for drive to be ready */
    if (ata_wait_ready() < 0) {
        return -1;
    }
    
    /* Select drive with LBA mode */
    outb(ata_io_base + ATA_REG_DRIVE, ATA_DRIVE_MASTER | 0x40 | ((lba >> 24) & 0x0F));
    io_wait();
    
    /* Set sector count and LBA */
    outb(ata_io_base + ATA_REG_SECCOUNT, count);
    outb(ata_io_base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(ata_io_base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ata_io_base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    /* Send WRITE command */
    outb(ata_io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    /* Write sectors */
    const uint16_t* buf = (const uint16_t*)buffer;
    for (int sector = 0; sector < count; sector++) {
        /* Wait for ready */
        if (ata_wait_drq() < 0) {
            return -1;
        }
        
        /* Write 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            outw(ata_io_base + ATA_REG_DATA, buf[sector * 256 + i]);
        }
        
        /* Flush cache */
        io_wait();
    }
    
    /* Wait for write to complete */
    ata_wait_ready();
    
    return 0;
}

