#pragma once

#define PTX_SHIFT 12  // offset of PTX in a linear address
#define PDX_SHIFT 22  // offset of PDX in a linear address

// page directory index
#define PDX(la) ((((uintptr_t)(la)) >> PDX_SHIFT) & 0x3FF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTX_SHIFT) & 0x3FF)

#define PTE_ADDR(pte) ((uintptr_t)(pte) & ~0xFFF)

#define PDE_ADDR(pde) PTE_ADDR(pde)