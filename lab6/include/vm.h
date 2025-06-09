#ifndef _VM_H
#define _VM_H

#include "mmu.h"
#include "types.h"

// Virtual address breakdown for 4-level page table
#define PD_IDX_MASK         0b111111111          // Mask for page descriptor index (9 bits)
#define PHY_ADDR_MASK       0x00000FFFFFFFF000   // Mask for physical address in page table entry (48 bits, 4KB aligned)
#define OFFSET_MASK         0x0000000000000FFF   // Mask for offset within a page (12 bits)

// Extract page table indices from virtual address
#define PGD_IDX(va) (((va) >> 39) & PD_IDX_MASK)  // Level 0 PGD (bits 47-39)
#define PUD_IDX(va) (((va) >> 30) & PD_IDX_MASK)  // Level 1 PUD (bits 38-30)
#define PMD_IDX(va) (((va) >> 21) & PD_IDX_MASK)  // Level 2 PMD (bits 29-21)
#define PTE_IDX(va) (((va) >> 12) & PD_IDX_MASK)  // Level 3 PTE (bits 20-12)

// User space memory layout
#define USER_CODE_BASE    0x0000000000000000    // User code starts at VA 0x0
#define USER_STACK_BASE   0x0000ffffffffb000    // User stack starts at VA 0x0000ffffffffb000
#define USER_STACK_TOP    0x0000fffffffff000    // User Stack top is at VA 0x0000fffffffff000
#define USER_STACK_SIZE   (4 * PAGE_SIZE)       // 4 pages (16KB) for stack

int mappages(unsigned long* pagetable, unsigned long va, unsigned long size, unsigned long pa);
unsigned long *walk(unsigned long* pagetable, unsigned long va);
int clear_pagetable(unsigned long* pagetable);

#endif
