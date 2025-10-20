# Page Swap Implementation Guide

## Overview

This document describes the page swap (paging) implementation in Zonix OS, which allows the system to use disk space as extended memory when physical RAM is full.

## Architecture

### Components

1. **swap.c/swap.h** - Core swap management
2. **swap_fifo.c/swap_fifo.h** - FIFO page replacement algorithm
3. **swap_clock.c/swap_clock.h** - Clock (Second Chance) algorithm
4. **swap_lru.c/swap_lru.h** - LRU (Least Recently Used) algorithm

### Swap Manager Interface

```c
typedef struct {
    const char *name;
    int (*init)(void);                     // Initialize swap manager
    int (*init_mm)(mm_struct *mm);         // Initialize mm struct for swap
    int (*map_swappable)(mm_struct *mm, uintptr_t addr, PageDesc *page, int swap_in);
    int (*swap_out_victim)(mm_struct *mm, PageDesc **page_ptr, int in_tick);
    int (*check_swap)(void);               // Verify correctness
} swap_manager;
```

## Page Replacement Algorithms

### 1. FIFO (First In First Out)

**Algorithm**: Pages are organized in a queue. The oldest page (first in) is selected for replacement.

**Pros**:
- Simple to implement
- Low overhead
- Fair to all pages

**Cons**:
- May replace frequently used pages
- Suffers from Belady's anomaly
- Poor performance for many workloads

**Use case**: Simple systems, predictable access patterns

### 2. Clock (Second Chance)

**Algorithm**: Uses a circular list with a "clock hand" pointer. Pages get a second chance before being replaced based on their accessed bit.

**How it works**:
1. Clock hand scans pages in circular order
2. If page's accessed bit is set, clear it and move to next
3. If accessed bit is clear, select page as victim
4. Gives recently accessed pages a "second chance"

**Pros**:
- Better than FIFO
- Approximates LRU with lower overhead
- Uses hardware accessed bit efficiently

**Cons**:
- Requires hardware support (PTE_A bit)
- Still may replace useful pages

**Use case**: General-purpose OS with hardware support

### 3. LRU (Least Recently Used)

**Algorithm**: Replaces the page that hasn't been used for the longest time.

**How it works**:
1. Maintain pages in order of access time
2. Most recently used pages at back of list
3. Least recently used page at front
4. On access, move page to back of list

**Pros**:
- Excellent performance
- Keeps frequently used pages in memory
- Matches locality principle

**Cons**:
- Higher overhead (list manipulation)
- Need to track every page access
- Complex to implement efficiently

**Use case**: Performance-critical systems with spare CPU cycles

## Usage

### Initialization

```c
// In kern/init.c or pmm_init()
swap_init();

// For each mm_struct
swap_init_mm(mm);
```

### Switching Algorithms

In `swap.c`, change the initialization:

```c
// For FIFO (default)
swap_mgr = &swap_mgr_fifo;

// For Clock
swap_mgr = &swap_mgr_clock;

// For LRU
swap_mgr = &swap_mgr_lru;
```

### Page Fault Handling

When a page fault occurs for a swapped-out page:

```c
// In page fault handler
if (page is swapped out) {
    PageDesc *page;
    swap_in(mm, addr, &page);
    page_insert(mm->pgdir, page, addr, perm);
    swap_mgr->map_swappable(mm, addr, page, 1);
}
```

### Swapping Out Pages

When memory is low:

```c
// Swap out n pages
int swapped = swap_out(mm, n, 0);
```

## Integration with VMM

### Page Table Entry Format

For swapped-out pages, PTE stores swap entry instead of physical address:

```
Present bit = 0 (page not in memory)
Bits 1-31: Swap entry (disk sector/offset)
```

### Page Fault Flow

```
1. Page fault occurs
2. Check if PTE present bit is 0
3. If swap entry exists in PTE:
   a. Allocate new physical page
   b. Read page from disk (swap_in)
   c. Update PTE to point to physical page
   d. Add page to swappable list
   e. Resume execution
4. Else:
   Handle as normal page fault
```

## Implementation Status

### ✅ Completed
- [x] Basic swap framework
- [x] Swap manager interface
- [x] FIFO algorithm implementation
- [x] Clock algorithm implementation
- [x] LRU algorithm implementation
- [x] swap_in() framework
- [x] swap_out() framework

### 🔄 TODO
- [ ] Disk I/O integration (swapfs_read/write)
- [ ] Reverse page mapping (find VA from page)
- [ ] PTE accessed bit tracking for Clock
- [ ] Performance statistics
- [ ] Swap space management
- [ ] Multiple swap devices support

## Testing

### Basic Test

```c
// Test swap manager initialization
void test_swap() {
    cprintf("Testing swap manager: %s\n", swap_mgr->name);
    
    // Initialize
    swap_init();
    
    // Create mm_struct
    mm_struct *mm = ...;
    swap_init_mm(mm);
    
    // Allocate pages and make them swappable
    for (int i = 0; i < N; i++) {
        PageDesc *page = alloc_pages(1);
        swap_mgr->map_swappable(mm, addr + i * PGSIZE, page, 0);
    }
    
    // Trigger swap out
    swap_out(mm, N/2, 0);
    
    // Trigger swap in
    PageDesc *page;
    swap_in(mm, addr, &page);
}
```

### Performance Comparison

You can compare algorithms by:
1. Counting page faults
2. Measuring swap I/O operations
3. Tracking hit rates

## Advanced Topics

### Enhanced Second Chance (NRU)

Enhance Clock algorithm by considering both accessed and modified bits:
- Class 0: Not accessed, not modified (best to replace)
- Class 1: Not accessed, modified
- Class 2: Accessed, not modified
- Class 3: Accessed, modified (keep in memory)

### Working Set Model

Track the "working set" of each process and keep those pages in memory.

### Page Frame Allocation

Different strategies:
- Equal allocation: Each process gets equal pages
- Proportional allocation: Based on process size
- Priority allocation: Based on process priority

## References

- Operating System Concepts (Silberschatz)
- Linux kernel mm/swap.c
- xv6 memory management
- OSDev Wiki: Paging

## Files Modified/Created

```
kern/mm/swap.h          - Updated with new interfaces
kern/mm/swap.c          - Implemented swap_in/swap_out
kern/mm/swap_fifo.c     - Complete FIFO implementation
kern/mm/swap_fifo.h     - FIFO header
kern/mm/swap_clock.c    - NEW: Clock algorithm
kern/mm/swap_clock.h    - NEW: Clock header
kern/mm/swap_lru.c      - NEW: LRU algorithm
kern/mm/swap_lru.h      - NEW: LRU header
```

## Next Steps

1. **Integrate with disk driver** (`kern/drivers/hd.c`)
2. **Implement swapfs_read/write** for actual disk I/O
3. **Add reverse mapping** to find VAs for physical pages
4. **Test with real workloads** and measure performance
5. **Add statistics** (page faults, swap ins/outs)
6. **Tune parameters** (e.g., how many pages to swap at once)

---

**Author**: Zonix OS Team  
**Date**: 2025-10-14  
**Status**: Initial Implementation
