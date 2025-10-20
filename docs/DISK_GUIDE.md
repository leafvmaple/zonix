# Zonix OS Disk Management Guide

## Overview

This document describes the disk management system in Zonix OS, including the IDE/ATA hard disk driver, block device abstraction layer, and integration with the swap system.

## Architecture

The disk management system is organized in three layers:

```
┌─────────────────────────────────────┐
│     Applications / Shell Commands   │
│   (lsblk, hdparm, disktest, dd)    │
├─────────────────────────────────────┤
│    Block Device Abstraction Layer   │
│         (blk.c/blk.h)              │
├─────────────────────────────────────┤
│      IDE/ATA Disk Driver           │
│         (hd.c/hd.h)                │
├─────────────────────────────────────┤
│          Hardware (IDE)             │
└─────────────────────────────────────┘
```

### Layer 1: IDE/ATA Disk Driver

**Files**: `kern/drivers/hd.c`, `kern/drivers/hd.h`

The lowest layer provides direct hardware access to IDE/ATA disks.

**Key Functions**:
- `hd_init()` - Initialize disk and detect geometry
- `hd_read(sector, buffer, count)` - Read sectors from disk
- `hd_write(sector, buffer, count)` - Write sectors to disk
- `hd_get_info()` - Get disk information
- `hd_test()` - Self-test for disk operations

**Features**:
- LBA (Logical Block Addressing) mode
- 28-bit addressing (up to 128 GB)
- Sector size: 512 bytes
- Primary IDE controller support (0x1F0)

**Hardware Interface**:
```
IDE Base: 0x1F0 (Primary Controller)
Ports:
  0x1F0 - Data register
  0x1F1 - Error/Features
  0x1F2 - Sector count
  0x1F3 - LBA low (bits 0-7)
  0x1F4 - LBA mid (bits 8-15)
  0x1F5 - LBA high (bits 16-23)
  0x1F6 - Device select (+ LBA bits 24-27)
  0x1F7 - Status/Command
```

### Layer 2: Block Device Abstraction

**Files**: `kern/drivers/blk.c`, `kern/drivers/blk.h`

Provides a unified interface for all block devices.

**Key Structures**:
```c
typedef struct block_device {
    int type;                    // BLK_TYPE_DISK or BLK_TYPE_SWAP
    uint32_t size;              // Size in blocks
    const char *name;           // Device name
    
    // Operations
    int (*read)(struct block_device *dev, uint32_t blockno, 
                void *buf, size_t nblocks);
    int (*write)(struct block_device *dev, uint32_t blockno, 
                 const void *buf, size_t nblocks);
} block_device_t;
```

**Key Functions**:
- `blk_init()` - Initialize block layer and register devices
- `blk_register(dev)` - Register a block device
- `blk_get_device(type)` - Get device by type
- `blk_read()` - Generic block read
- `blk_write()` - Generic block write
- `blk_list_devices()` - List all registered devices

### Layer 3: Swap Integration

**Files**: `kern/mm/swap.c`, `kern/mm/swap.h`

Integrates disk with the page swap system.

**Swap Space Layout**:
```
Disk Layout:
┌──────────────────────────────────────┐
│ Sectors 0-999: General use           │
├──────────────────────────────────────┤
│ Sector 1000+: Swap space             │
│   - Each page: 8 sectors (4KB)       │
│   - Swap entry maps to disk location │
└──────────────────────────────────────┘
```

**Key Functions**:
- `swapfs_init()` - Initialize swap filesystem
- `swapfs_read(entry, page)` - Read page from swap
- `swapfs_write(entry, page)` - Write page to swap

**Swap Entry Format**:
```
32-bit swap entry:
┌────────┬────────────────────────┐
│ 8 bits │    24 bits             │
│ flags  │    page offset         │
└────────┴────────────────────────┘

Disk sector = SWAP_START_SECTOR + (offset * 8)
```

## API Reference

### Basic Disk Operations

#### Read Sectors
```c
int hd_read(uint32_t secno, void *dst, size_t nsecs);
```
- **Parameters**:
  - `secno`: Starting sector number (LBA)
  - `dst`: Destination buffer (must be at least `nsecs * 512` bytes)
  - `nsecs`: Number of sectors to read
- **Returns**: 0 on success, -1 on error
- **Example**:
```c
uint8_t buffer[512];
if (hd_read(100, buffer, 1) == 0) {
    // Successfully read sector 100
}
```

#### Write Sectors
```c
int hd_write(uint32_t secno, const void *src, size_t nsecs);
```
- **Parameters**:
  - `secno`: Starting sector number (LBA)
  - `src`: Source buffer
  - `nsecs`: Number of sectors to write
- **Returns**: 0 on success, -1 on error

#### Get Disk Info
```c
int hd_get_info(disk_info_t *info);

typedef struct {
    uint32_t size;        // Total sectors
    uint16_t cylinders;   // CHS geometry
    uint16_t heads;
    uint16_t sectors;
    int valid;            // Device valid flag
} disk_info_t;
```

### Block Device Operations

#### Read Blocks
```c
int blk_read(block_device_t *dev, uint32_t blockno, 
             void *buf, size_t nblocks);
```

#### Write Blocks
```c
int blk_write(block_device_t *dev, uint32_t blockno, 
              const void *buf, size_t nblocks);
```

#### Get Device
```c
block_device_t *blk_get_device(int type);
// type: BLK_TYPE_DISK or BLK_TYPE_SWAP
```

## Shell Commands

Zonix provides Linux-like commands for disk management:

### lsblk - List Block Devices
```bash
zonix> lsblk
Block devices:
  hd0: type=disk, size=204800 blocks (100 MB)
```

### hdparm - Show Disk Information
```bash
zonix> hdparm
Disk information:
  Size: 204800 sectors (100 MB)
  CHS: 400 cylinders, 16 heads, 32 sectors/track
```

### disktest - Test Disk I/O
```bash
zonix> disktest
=== Disk Test ===
Test 1: Write sector 100...
  OK
Test 2: Read sector 100...
  OK
Test 3: Verify data...
  OK
=== Disk Test Complete ===
```

### swaptest - Test Swap with Disk
```bash
zonix> swaptest
[Runs comprehensive swap tests including disk I/O]
```

## Testing

### Running Tests

1. **Build and Run**:
```bash
cd /path/to/zonix
make clean && make
./tests/run_disk_tests.sh
```

2. **Manual Testing in Shell**:
```bash
# After booting Zonix
zonix> lsblk      # List devices
zonix> hdparm     # Show disk info
zonix> disktest   # Test disk I/O
zonix> swaptest   # Test swap
```

### Test Coverage

1. **Hardware Layer Tests**:
   - Disk detection and initialization
   - Sector read operations
   - Sector write operations
   - Data integrity verification
   - Error handling

2. **Block Device Tests**:
   - Device registration
   - Device enumeration
   - Read/write through abstraction layer

3. **Swap Integration Tests**:
   - Swap space initialization
   - Page swap out to disk
   - Page swap in from disk
   - Swap entry management

### Expected Output

All tests should produce "OK" status:
```
=== Disk Test ===
Test 1: Write sector 100...
  OK
Test 2: Read sector 100...
  OK
Test 3: Verify data...
  OK
=== Disk Test Complete ===
```

## Configuration

### Disk Parameters

Edit `kern/drivers/hd.h`:
```c
#define IDE0_BASE     0x1F0    // Primary IDE base
#define SECTOR_SIZE   512      // Bytes per sector
```

### Swap Configuration

Edit `kern/mm/swap.c`:
```c
#define SWAP_START_SECTOR  1000        // Swap start sector
#define SECTORS_PER_PAGE   8           // Sectors per page (4KB/512B)
```

## Troubleshooting

### Disk Not Detected

**Symptom**: `hd_init: disk not ready`

**Solutions**:
- Check if disk image exists in bochs configuration
- Verify IDE controller is enabled
- Check bochs disk configuration

### Read/Write Failures

**Symptom**: `hd_read: disk read failed`

**Solutions**:
- Check sector number is within disk bounds
- Verify buffer alignment
- Check disk status register for errors

### Swap Failures

**Symptom**: `swapfs_init: no disk device available`

**Solutions**:
- Ensure `blk_init()` is called before `swap_init()`
- Check disk initialization succeeded
- Verify disk device is registered

## Implementation Notes

### Performance Considerations

1. **Blocking I/O**: Current implementation uses polling (blocking)
2. **No Caching**: Direct disk access without buffer cache
3. **Single Request**: No I/O queue or scheduling

### Future Enhancements

1. **Interrupt-driven I/O**: Replace polling with interrupts
2. **DMA Support**: Use DMA for faster transfers
3. **Buffer Cache**: Add disk block caching
4. **Multiple Disks**: Support secondary IDE controller
5. **File System**: Add simple file system on top of block layer
6. **AHCI Support**: Modern SATA controller support

### Educational Focus

This implementation prioritizes:
- **Simplicity**: Easy to understand code
- **Clarity**: Well-commented and documented
- **Completeness**: Full working implementation
- **Testing**: Comprehensive test coverage

## References

- Intel 8086/8088 Family Architecture
- ATA/ATAPI-8 Specification
- Linux Block Device Layer
- xv6 Operating System (MIT)

## See Also

- `SWAP_GUIDE.md` - Page swap implementation
- `SWAP_TESTING.md` - Swap testing procedures
- `README.md` - General OS documentation
