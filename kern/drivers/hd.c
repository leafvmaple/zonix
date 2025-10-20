#include "hd.h"
#include "stdio.h"

#include <arch/x86/io.h>
#include <arch/x86/drivers/i8259.h>
#include "pic.h"

// Global IDE devices
static ide_device_t ide_devices[MAX_IDE_DEVICES];
static int num_devices = 0;

// Device initialization configurations
static const struct {
    uint8_t channel;
    uint8_t drive;
    uint16_t base;
    uint8_t irq;
    const char *name;
} ide_configs[MAX_IDE_DEVICES] = {
    {0, 0, IDE0_BASE, IRQ_IDE1, "hda"},  // Primary Master
    {0, 1, IDE0_BASE, IRQ_IDE1, "hdb"},  // Primary Slave
    {1, 0, IDE1_BASE, IRQ_IDE2, "hdc"},  // Secondary Master
    {1, 1, IDE1_BASE, IRQ_IDE2, "hdd"},  // Secondary Slave
};

/**
 * Wait for disk to be ready (on specific base port)
 */
static int hd_wait_ready_on_base(uint16_t base) {
    int timeout = 100000;
    
    while (timeout-- > 0) {
        uint8_t status = inb(base + IDE_STATUS);
        
        // Check if busy bit is clear and ready bit is set
        if ((status & (IDE_BSY | IDE_DRDY)) == IDE_DRDY) {
            return 0;
        }
    }
    
    return -1;
}



/**
 * Wait for disk to be ready to transfer data (on specific base port)
 */
static int hd_wait_data_on_base(uint16_t base) {
    int timeout = 100000;
    
    while (timeout-- > 0) {
        uint8_t status = inb(base + IDE_STATUS);
        
        // Check if busy bit is clear and data request bit is set
        if ((status & (IDE_BSY | IDE_DRQ)) == IDE_DRQ) {
            return 0;
        }
        
        // Check for error
        if (status & IDE_ERR) {
            return -1;
        }
    }
    
    return -1;
}



/**
 * Detect a single IDE device
 */
static int hd_detect_device(int dev_id) {
    ide_device_t *dev = &ide_devices[dev_id];
    uint16_t base = ide_configs[dev_id].base;
    uint8_t drive_sel = ide_configs[dev_id].drive ? IDE_DEV_SLAVE : IDE_DEV_MASTER;
    
    // Select device
    outb(base + IDE_DEVICE, drive_sel);
    
    // Wait 400ns for device selection (read status 4 times)
    for (int i = 0; i < 4; i++) {
        inb(base + IDE_STATUS);
    }
    
    // Send IDENTIFY command
    outb(base + IDE_COMMAND, IDE_CMD_IDENTIFY);
    
    // Check if device exists (status != 0 and != 0xFF)
    uint8_t status = inb(base + IDE_STATUS);
    if (status == 0 || status == 0xFF) {
        return -1;  // No device
    }
    
    // Wait for data
    if (hd_wait_data_on_base(base) != 0) {
        return -1;
    }
    
    // Read identification data
    uint16_t buf[256];
    insw(base + IDE_DATA, buf, 256);
    
    // Fill device structure
    dev->channel = ide_configs[dev_id].channel;
    dev->drive = ide_configs[dev_id].drive;
    dev->base = base;
    dev->irq = ide_configs[dev_id].irq;
    dev->info.cylinders = buf[1];
    dev->info.heads = buf[3];
    dev->info.sectors = buf[6];
    dev->info.size = *((uint32_t *)&buf[60]);
    
    if (dev->info.size == 0) {
        dev->info.size = dev->info.cylinders * dev->info.heads * dev->info.sectors;
    }
    
    dev->info.valid = 1;
    dev->present = 1;
    
    // Copy device name
    for (int i = 0; i < IDE_NAME_LEN; i++) {
        dev->name[i] = ide_configs[dev_id].name[i];
    }
    dev->name[IDE_NAME_LEN - 1] = '\0';
    
    return 0;
}

/**
 * Initialize all IDE devices
 */
void hd_init(void) {
    // Enable IDE interrupts for both channels
    pic_enable(IRQ_IDE1);
    pic_enable(IRQ_IDE2);
    
    // Clear device array
    for (int i = 0; i < MAX_IDE_DEVICES; i++) {
        ide_devices[i].present = 0;
        ide_devices[i].info.valid = 0;
    }
    
    num_devices = 0;
    
    // Try to detect all 4 possible devices
    for (int i = 0; i < MAX_IDE_DEVICES; i++) {
        if (hd_detect_device(i) == 0) {
            num_devices++;
        }
    }
    
    cprintf("hd_init: found %d device(s)\n", num_devices);
}

/**
 * Read sectors from specific device
 */
int hd_read_device(int dev_id, uint32_t secno, void *dst, size_t nsecs) {
    ide_device_t *dev = &ide_devices[dev_id];
    if (!dev->present) {
        return -1;
    }
    
    if (secno + nsecs > dev->info.size) {
        return -1;
    }
    
    uint16_t base = dev->base;
    uint8_t drive_sel = dev->drive ? IDE_DEV_SLAVE : IDE_DEV_MASTER;
    
    // Read sectors one by one
    for (size_t i = 0; i < nsecs; i++) {
        uint32_t lba = secno + i;
        
        // Wait for disk ready
        if (hd_wait_ready_on_base(base) != 0) {
            return -1;
        }
        
        // Set sector count to 1
        outb(base + IDE_SECTOR_COUNT, 1);
        
        // Set LBA address
        outb(base + IDE_LBA_LOW, lba & 0xFF);
        outb(base + IDE_LBA_MID, (lba >> 8) & 0xFF);
        outb(base + IDE_LBA_HIGH, (lba >> 16) & 0xFF);
        
        // Set device (LBA mode, bits 24-27 of LBA)
        outb(base + IDE_DEVICE, drive_sel | ((lba >> 24) & 0x0F));
        
        // Send read command
        outb(base + IDE_COMMAND, IDE_CMD_READ);
        
        // Wait for data
        if (hd_wait_data_on_base(base) != 0) {
            return -1;
        }
        
        // Read data
        void *buf = (void *)((uint8_t *)dst + i * SECTOR_SIZE);
        insw(base + IDE_DATA, buf, SECTOR_SIZE / 2);
    }
    
    return 0;
}

/**
 * Write sectors to specific device
 */
int hd_write_device(int dev_id, uint32_t secno, const void *src, size_t nsecs) {
    ide_device_t *dev = &ide_devices[dev_id];
    if (!dev->present) {
        return -1;
    }
    
    if (secno + nsecs > dev->info.size) {
        return -1;
    }
    
    uint16_t base = dev->base;
    uint8_t drive_sel = dev->drive ? IDE_DEV_SLAVE : IDE_DEV_MASTER;
    
    // Write sectors one by one
    for (size_t i = 0; i < nsecs; i++) {
        uint32_t lba = secno + i;
        
        // Wait for disk ready
        if (hd_wait_ready_on_base(base) != 0) {
            return -1;
        }
        
        // Set sector count to 1
        outb(base + IDE_SECTOR_COUNT, 1);
        
        // Set LBA address
        outb(base + IDE_LBA_LOW, lba & 0xFF);
        outb(base + IDE_LBA_MID, (lba >> 8) & 0xFF);
        outb(base + IDE_LBA_HIGH, (lba >> 16) & 0xFF);
        
        // Set device (LBA mode, bits 24-27 of LBA)
        outb(base + IDE_DEVICE, drive_sel | ((lba >> 24) & 0x0F));
        
        // Send write command
        outb(base + IDE_COMMAND, IDE_CMD_WRITE);
        
        // Wait for disk ready to receive data
        if (hd_wait_data_on_base(base) != 0) {
            return -1;
        }
        
        // Write data
        const void *buf = (const void *)((const uint8_t *)src + i * SECTOR_SIZE);
        outsw(base + IDE_DATA, buf, SECTOR_SIZE / 2);
        
        // Wait for write to complete
        if (hd_wait_ready_on_base(base) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * Get device by ID
 */
ide_device_t *hd_get_device(int dev_id) {
    if (!ide_devices[dev_id].present) {
        return NULL;
    }
    
    return &ide_devices[dev_id];
}

/**
 * Get number of detected devices
 */
int hd_get_device_count(void) {
    return num_devices;
}

/**
 * Test disk read/write for all devices
 */
void hd_test(void) {
    cprintf("\n=== Multi-Disk Test ===\n");
    cprintf("Testing %d disk device(s)\n\n", num_devices);
    
    if (num_devices == 0) {
        cprintf("No disk devices found!\n");
        cprintf("=== Test Complete ===\n\n");
        return;
    }
    
    // Allocate test buffers
    static uint8_t write_buf[SECTOR_SIZE];
    static uint8_t read_buf[SECTOR_SIZE];
    
    // Test each device
    for (int dev_id = 0; dev_id < MAX_IDE_DEVICES; dev_id++) {
        ide_device_t *dev = &ide_devices[dev_id];
        
        if (!dev->present) {
            continue;
        }
        
        cprintf("--- Testing %s (dev_id=%d) ---\n", dev->name, dev_id);
        cprintf("  Size: %d sectors (%d MB)\n", dev->info.size, dev->info.size / 2048);
        
        // Fill write buffer with test pattern (unique per device)
        for (int i = 0; i < SECTOR_SIZE; i++) {
            write_buf[i] = (uint8_t)((i + dev_id * 17) & 0xFF);
        }
        
        // Use sector 100 for testing
        uint32_t test_sector = 100;
        
        cprintf("  Test 1: Write sector %d...\n", test_sector);
        if (hd_write_device(dev_id, test_sector, write_buf, 1) != 0) {
            cprintf("    FAILED: write error\n");
            continue;
        }
        cprintf("    OK\n");
        
        cprintf("  Test 2: Read sector %d...\n", test_sector);
        if (hd_read_device(dev_id, test_sector, read_buf, 1) != 0) {
            cprintf("    FAILED: read error\n");
            continue;
        }
        cprintf("    OK\n");
        
        cprintf("  Test 3: Verify data...\n");
        int errors = 0;
        for (int i = 0; i < SECTOR_SIZE; i++) {
            if (read_buf[i] != write_buf[i]) {
                if (errors < 5) {
                    cprintf("    Mismatch at offset %d: expected 0x%02x, got 0x%02x\n", 
                           i, write_buf[i], read_buf[i]);
                }
                errors++;
            }
        }
        
        if (errors > 0) {
            cprintf("    FAILED: %d mismatches\n", errors);
        } else {
            cprintf("    OK\n");
        }
        
        cprintf("  %s test %s\n\n", dev->name, errors == 0 ? "PASSED" : "FAILED");
    }
    
    cprintf("=== Multi-Disk Test Complete ===\n\n");
}