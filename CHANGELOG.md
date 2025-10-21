# Changelog

All notable changes to the Zonix Operating System project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2025-10-21

### Added
- **Process Scheduling**: Implemented complete round-robin scheduler
  - `schedule()`: Find and switch to next runnable process
  - `proc_run()`: Context switch with CR3 and register switching
  - `switch_to()`: Assembly routine for low-level context switch
- **Process Context Switch**: Working idle ↔ init process switching
  - Fixed `current` pointer update timing (before `switch_to`)
  - Fixed `trapret` to match trapframe structure (removed invalid segment register pops)
  - Fixed `forkret` to directly jump without extra `call`

### Fixed
- **Critical Scheduling Bug**: Process switching now works correctly
  - Updated `current` pointer **before** `switch_to()` (not after)
  - Fixed trapframe restoration in `forkret`/`trapret`
  - Removed bogus segment register pops that corrupted stack
- **Shell Prompt Timing**: Prompt now displays after full system initialization
  - Moved prompt to init process (executes after all initialization complete)

### Technical Details
- Context structure: eip, esp, ebx, ecx, edx, esi, edi, ebp
- Trapframe structure: 8 general regs + trapno + errcode + eip + cs + eflags + esp + ss
- Init process (PID 1) yields CPU via continuous `schedule()` calls

## [0.2.2] - 2025-10-21

### Fixed
- **BSS Segment Initialization**: Fixed uninitialized global/static variables
  - Added BSS clearing code in `init/head.S` (follows Linux kernel pattern)
  - Defined `__bss_start` and `__bss_end` symbols in linker script
  - Root cause: Bootloader loads segments but doesn't zero BSS
- **Debug Support**: Added Bochs port e9 output to `cons_putc()` for logging

## [0.2.1] - 2025-10-21

### Added
- **Init Process (PID 1)**: Created via `do_fork()` with `init_main()` entry point
- **Hash Table Lookup**: `find_proc()` for O(1) PID lookup (25-50x faster than list traversal)
- **List Helper**: `list_prev()` for backwards traversal

### Changed
- Renamed `PID_HASH()` to `pid_hashfn()` for Linux naming consistency
- Removed `NR_TASKS` limit for dynamic process creation
- Fixed `print_all_procs()` ordering with `list_prev()`

### Improved
- All processes created through unified `do_fork()` path
- Simplified `do_fork()` error handling

## [0.2.0] - 2025-10-21

### Added
- **Process Management System**
  - Complete `task_struct` with 18 fields (states, relationships, context)
  - Idle process (PID 0) and process lifecycle functions
  - Context switching in `kern/sched/switch.S`
  - Hash table (1024 buckets) and process list
  - `proc_get_cr3()` for dynamic page directory lookup
- **Shell Commands**
  - `ps` command: Display process info (PID, STATE, PPID, KSTACK, MM, NAME)
- **Console I/O**
  - Signed integer support in `cprintf()` with `%d` format
  - Proper negative number handling with padding
- **Memory Management**
  - Physical memory manager with first-fit algorithm
  - Virtual memory with page table management
  - Swap system with FIFO replacement policy
  - Demand paging and page fault handling
- **Interrupt & Trap**
  - IDT initialization with 256 entries
  - Trap frame for interrupt context
  - Page fault handler (interrupt 14)
- **Drivers**
  - CGA text mode display (80x25)
  - Keyboard input via i8042 controller
  - PIT timer for scheduling
  - ATA hard disk driver (up to 4 disks)

### Changed
- Updated memory manager to use `init_mm` as kernel address space
- Refactored page table operations for clarity

## [0.1.0] - 2025-10-20

### Added
- **Basic Kernel Infrastructure**
  - Bootloader with ELF loading
  - Protected mode with GDT/IDT
  - Simple page table (4MB mapping)
  - Basic console I/O (`cprintf`, CGA driver)
  - Interrupt handling framework
- **Build System**
  - Makefile with separate compilation
  - Bochs configuration for testing
  - Linker scripts for bootloader and kernel

### Initial Release
First working kernel that boots and displays output.
