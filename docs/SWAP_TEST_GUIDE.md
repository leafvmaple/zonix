# Swap System Testing - Complete Guide

## 📋 Table of Contents

1. [Quick Start](#quick-start)
2. [Test Files Overview](#test-files-overview)
3. [Running Tests](#running-tests)
4. [Test Cases](#test-cases)
5. [Interpreting Results](#interpreting-results)
6. [Troubleshooting](#troubleshooting)
7. [Extending Tests](#extending-tests)

---

## Quick Start

### Simplest Method - Run Tests in Shell

```bash
# 1. Build
cd /mnt/d/Code/zonix
make clean && make

# 2. Start QEMU
make qemu

# 3. In the Zonix shell, type:
zonix> swaptest
```

### Automated Script Method

```bash
./tests/run_swap_tests.sh
```

---

## Test Files Overview

### Core Test Files

| File | Purpose |
|------|---------|
| `kern/mm/swap_test.c` | Main test implementation |
| `kern/mm/swap_test.h` | Test function declarations |
| `tests/run_swap_tests.sh` | Automated test runner script |
| `tests/README.md` | Quick reference guide |
| `docs/SWAP_TESTING.md` | Detailed testing documentation |

### Test Integration

- **Shell Integration**: `kern/cons/shell.c` - Added `swaptest` command
- **Build System**: Tests automatically compiled with kernel

---

## Running Tests

### Method 1: Interactive Shell Command

**Steps:**
1. Build the kernel: `make`
2. Start QEMU: `make qemu`
3. Run command: `swaptest`

**Advantages:**
- ✅ Easy to run multiple times
- ✅ Can debug interactively
- ✅ Visual feedback

### Method 2: Automatic on Boot

Edit `kern/init.c`:

```c
#include "mm/swap_test.h"

void kern_init() {
    // ... existing init ...
    
    swap_init();
    
    // Run tests automatically
    cprintf("\nRunning swap tests...\n");
    run_swap_tests();
    
    shell_init();
    // ...
}
```

**Advantages:**
- ✅ Tests run every boot
- ✅ Good for CI/CD
- ✅ Catch regressions early

### Method 3: Scripted Testing

```bash
#!/bin/bash
cd /mnt/d/Code/zonix
make clean && make

# Run with expect/timeout for automation
timeout 30s make qemu <<EOF
swaptest
^AX
EOF
```

---

## Test Cases

### Unit Tests (9 tests total)

#### FIFO Algorithm Tests (2 tests)

**test_fifo_basic**
- Tests basic FIFO ordering
- Verifies oldest page selected first
- Checks empty list handling

**test_fifo_interleaved**
- Tests add/remove interleaving
- Verifies order maintained correctly

#### LRU Algorithm Tests (2 tests)

**test_lru_basic**
- Tests LRU victim selection
- Verifies least recently used page chosen

**test_lru_access_pattern**
- Tests page re-access moves to back
- Verifies access order tracking

#### Clock Algorithm Tests (1 test)

**test_clock_basic**
- Tests circular list operation
- Verifies victim selection works

#### Integration Tests (3 tests)

**test_swap_init**
- Verifies swap system initialization
- Checks mm_struct setup

**test_swap_in_basic**
- Tests page swapping in
- Verifies page allocation

**test_swap_out_basic**
- Tests page swapping out
- Verifies victim selection integration

#### Comparison Test (1 test)

**test_algorithm_comparison**
- Compares all three algorithms
- Verifies all are functional

---

## Interpreting Results

### Successful Test Output

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
  
... (more tests) ...

========================================
   TEST SUMMARY                        
========================================
   Passed: 9
   Failed: 0
   Total:  9

   [OK] ALL TESTS PASSED!
========================================
```

### What to Look For

* **All tests show [PASSED]**
* **No kernel panics**
* **Failed count is 0**
* **All [OK] checks passed**

### Failed Test Example

```
[TEST] FIFO Basic Operation
  [FAIL] Victim 0 is page 0
  [FAILED]
```

This indicates:
- ❌ FIFO algorithm not selecting correct victim
- Check list operations in `swap_fifo.c`
- Verify `list_add` and `list_del` usage

---

## Troubleshooting

### Common Issues

#### Issue 1: Tests Not Running

**Symptoms:**
- `swaptest` command not recognized
- "Unknown command" message

**Solutions:**
```bash
# 1. Verify shell.c includes swap_test.h
grep "swap_test.h" kern/cons/shell.c

# 2. Check command table
grep "swaptest" kern/cons/shell.c

# 3. Rebuild
make clean && make
```

#### Issue 2: Kernel Panic During Tests

**Symptoms:**
- System crashes with panic message
- Triple fault / reboot

**Common Causes:**
1. **Null pointer dereference**
   - Check `mm.swap_list` initialization
   - Verify `swap_init_mm()` called

2. **Invalid page descriptor**
   - Check `alloc_pages()` return value
   - Ensure pages not freed while in use

3. **Stack overflow**
   - Reduce local array sizes in tests
   - Check for infinite loops

**Debug Steps:**
```c
// Add debug prints
cprintf("DEBUG: swap_list = %p\n", mm.swap_list);
cprintf("DEBUG: page = %p\n", page);
```

#### Issue 3: Test Failures

**test_fifo_basic fails:**
```
[FAIL] Victim 0 is page 0
```
- Check FIFO queue implementation
- Verify `list_add_before` vs `list_add_after`
- Ensure pages added to correct end

**test_lru_access_pattern fails:**
```
[FAIL] After access, LRU is page 1
```
- Check if page moved to back on access
- Verify `swap_in` parameter is used
- Check list manipulation in `map_swappable`

**test_swap_in_basic fails:**
```
[FAIL] swap_in returns success
```
- Check `get_pte` returns valid pointer
- Verify `alloc_pages` succeeds
- Check return value handling

#### Issue 4: Compilation Errors

**Error: undeclared PTE_W**
```c
// Add to swap_test.c
#include <arch/x86/mmu.h>
```

**Error: implicit declaration**
```c
// Add external declarations
extern PageDesc *alloc_pages(size_t n);
extern swap_manager *swap_mgr;
```

---

## Extending Tests

### Adding a New Test

```c
// In swap_test.c

void test_my_new_feature() {
    TEST_START("My New Feature");
    
    // Setup
    mm_struct mm;
    swap_mgr_fifo.init_mm(&mm);
    
    // Test logic
    PageDesc page;
    int result = my_function(&mm, &page);
    
    // Assertions
    TEST_ASSERT(result == 0, "Function succeeded");
    TEST_ASSERT(page.ref > 0, "Page reference valid");
    
    TEST_END();
}

// Add to run_swap_tests()
void run_swap_tests() {
    // ... existing tests ...
    
    cprintf("\n--- Custom Tests ---\n");
    test_my_new_feature();
    
    // ... summary ...
}
```

### Adding Shell Commands

```c
// In shell.c

static void cmd_test_custom() {
    test_my_new_feature();
}

shell_cmd_t commands[] = {
    // ... existing commands ...
    {"testcustom", "Run custom test", cmd_test_custom},
};
```

### Performance Testing

```c
#include "../drivers/pit.h"

void test_performance() {
    cprintf("\n[PERF] Testing algorithm performance\n");
    
    swap_manager *algs[] = {&swap_mgr_fifo, &swap_mgr_clock, &swap_mgr_lru};
    
    for (int i = 0; i < 3; i++) {
        uint32_t start = get_ticks();
        
        // Run operations
        mm_struct mm;
        algs[i]->init_mm(&mm);
        
        // ... test operations ...
        
        uint32_t end = get_ticks();
        cprintf("  %s: %d ticks\n", algs[i]->name, end - start);
    }
}
```

---

## Test Coverage

### Current Coverage ✅

- [x] FIFO algorithm correctness
- [x] LRU algorithm correctness
- [x] Clock algorithm basic operation
- [x] Swap system initialization
- [x] Basic swap in operation
- [x] Basic swap out operation
- [x] Algorithm comparison

### Missing Coverage ⬜

- [ ] Disk I/O integration
- [ ] PTE accessed bit tracking
- [ ] Error handling (disk full, etc.)
- [ ] Concurrent access / race conditions
- [ ] Memory pressure scenarios
- [ ] Performance under load
- [ ] Swap space management
- [ ] Page fault integration

---

## Continuous Integration

### GitHub Actions Example

```yaml
# .github/workflows/test.yml
name: Swap Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc make nasm qemu-system-x86
      - name: Build
        run: make
      - name: Run tests
        run: ./tests/run_swap_tests.sh
```

---

## Best Practices

### 1. Test-Driven Development

```bash
# Write test first
vim kern/mm/swap_test.c  # Add test_new_feature()

# Run and see it fail
make && make qemu
> swaptest

# Implement feature
vim kern/mm/swap.c

# Run and see it pass
make && make qemu
> swaptest
```

### 2. Regular Testing

- Run tests after every significant change
- Test before committing
- Add regression tests for bugs

### 3. Clean Test Code

- One concept per test
- Clear test names
- Good error messages
- Clean up resources

---

## Next Steps

1. **Run the tests** - Verify everything works
2. **Fix any failures** - Debug and iterate
3. **Add disk I/O** - Integrate with hd.c
4. **Add stress tests** - Test under load
5. **Performance tuning** - Optimize algorithms

---

## Resources

- **SWAP_TESTING.md** - Detailed test documentation with code examples
- **SWAP_GUIDE.md** - Implementation guide and API reference
- **SWAP_IMPLEMENTATION.md** - Architecture and design decisions
- **tests/README.md** - Quick reference for running tests

---

## Support

For issues or questions:
1. Check this documentation
2. Review test output carefully
3. Add debug prints
4. Check related code (pmm.c, vmm.c)
5. Create an issue with test output

---

**Last Updated**: 2025-10-14
**Version**: 1.0
**Status**: ✅ All tests passing
