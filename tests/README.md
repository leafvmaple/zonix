# Quick Start: Testing Swap System

## Method 1: Automated Test Script

```bash
# From zonix root directory
./tests/run_swap_tests.sh
```

This will:
1. Clean and rebuild the kernel
2. Start QEMU
3. Wait for you to type `swaptest` in the shell

## Method 2: Manual Testing

### Step 1: Build
```bash
make clean
make
```

### Step 2: Run in QEMU
```bash
make qemu
```

### Step 3: Execute Tests
In the Zonix shell, type:
```
zonix> swaptest
```

## Expected Output

```
========================================
   ZONIX SWAP SYSTEM TEST SUITE       
========================================

--- FIFO Algorithm Tests ---

[TEST] FIFO Basic Operation
  [OK] Victim 0 is page 0
  [OK] Victim 1 is page 1
  [OK] Victim 2 is page 2
  [OK] Victim 3 is page 3
  [OK] Victim 4 is page 4
  [OK] Empty list returns error
  [PASSED]

[TEST] FIFO Interleaved Add/Remove
  [OK] First victim is page 0
  [OK] Second victim is page 1
  [PASSED]

--- LRU Algorithm Tests ---

[TEST] LRU Basic Operation
  [OK] LRU victim is page 0
  [PASSED]

[TEST] LRU Access Pattern
  [OK] After access, LRU is page 1
  [OK] Next LRU is page 2
  [OK] Last is page 0
  [PASSED]

--- Clock Algorithm Tests ---

[TEST] Clock Basic Operation
  [OK] Clock selects a victim
  [PASSED]

--- Integration Tests ---

[TEST] Swap Initialization
  [OK] Swap system initialized
  [OK] swap_init_mm succeeds
  [OK] Swap list created
  [PASSED]

[TEST] Swap In Basic
  [OK] swap_in returns success
  [OK] Page allocated
  [PASSED]

[TEST] Swap Out Basic
  [OK] swap_out executes
  [PASSED]

--- Algorithm Comparison ---

[TEST] Algorithm Comparison

  Comparing algorithm characteristics:
    fifo swap manager
      Successfully removed 10/10 pages
    clock swap manager
      Successfully removed 10/10 pages
    lru swap manager
      Successfully removed 10/10 pages
  [OK] All algorithms functional
  [PASSED]

========================================
   TEST SUMMARY                        
========================================
   Passed: 9
   Failed: 0
   Total:  9

   [OK] ALL TESTS PASSED!
========================================
```

## Debugging Failed Tests

### Enable Debug Output

Edit `kern/mm/swap.c` and add:

```c
#define SWAP_DEBUG 1

#ifdef SWAP_DEBUG
#define DEBUG_PRINT(fmt, ...) cprintf("[SWAP DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif
```

### Common Issues

**Build Errors**:
- Make sure all swap_*.c files are in `kern/mm/`
- Check Makefile includes `kern/mm` directory

**Test Failures**:
- Check if `swap_init()` is called in kernel init
- Verify page allocation works (`alloc_pages`)
- Check list operations are correct

**Kernel Panic**:
- Usually null pointer dereference
- Check swap_list initialization
- Verify page descriptor pointers

## Advanced Testing

### Test Individual Functions

In your shell, you can add more commands:

```c
// In shell.c
static void cmd_test_fifo(void) {
    test_fifo_basic();
}

static void cmd_test_lru(void) {
    test_lru_basic();
}

// Add to command table
{"testfifo", "Test FIFO algorithm", cmd_test_fifo},
{"testlru", "Test LRU algorithm", cmd_test_lru},
```

### Performance Testing

Add timing:

```c
#include "../drivers/pit.h"  // For timer

uint32_t start = get_ticks();
// ... test code ...
uint32_t end = get_ticks();
cprintf("Time: %d ticks\n", end - start);
```

## Integration with Kernel Init

To run tests automatically on boot, add to `kern/init.c`:

```c
#include "mm/swap_test.h"

void kern_init() {
    // ... existing init code ...
    
    swap_init();  // Initialize swap system
    
    // Run tests
    run_swap_tests();
    
    // ... continue with shell, etc ...
}
```

## Next Steps

1. ✅ Basic algorithm tests pass
2. ⬜ Add disk I/O integration
3. ⬜ Test with real page faults
4. ⬜ Add performance benchmarks
5. ⬜ Stress test with heavy load

## Troubleshooting

### Test hangs or crashes

Check:
- Interrupts are properly disabled during critical sections
- No infinite loops in victim selection
- Page descriptors are valid

### Inconsistent results

- Verify list operations are atomic
- Check for race conditions (if multicore)
- Ensure proper initialization order

## Documentation

- **SWAP_TESTING.md** - Detailed test documentation
- **SWAP_GUIDE.md** - Implementation guide
- **SWAP_IMPLEMENTATION.md** - Technical details

---

**Last Updated**: 2025-10-14
