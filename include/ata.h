/**
 * MiniOS - ATA/IDE Disk Driver Interface
 */

#ifndef _MINIOS_ATA_H
#define _MINIOS_ATA_H

#include "types.h"

/* Sector size in bytes */
#define ATA_SECTOR_SIZE 512

/**
 * Initialize the ATA driver
 * Detects and initializes primary IDE controller
 */
void ata_init(void);

/**
 * Read sectors from disk
 * 
 * @param lba     Starting logical block address
 * @param count   Number of sectors to read (1-256, 0 means 256)
 * @param buffer  Buffer to store read data (must be at least count * 512 bytes)
 * @return        0 on success, negative on error
 */
int ata_read_sectors(uint32_t lba, uint8_t count, void* buffer);

/**
 * Write sectors to disk
 * 
 * @param lba     Starting logical block address
 * @param count   Number of sectors to write (1-256, 0 means 256)
 * @param buffer  Buffer containing data to write
 * @return        0 on success, negative on error
 */
int ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer);

/**
 * Check if ATA drive is present
 * @return non-zero if drive is detected
 */
int ata_is_present(void);

#endif /* _MINIOS_ATA_H */

