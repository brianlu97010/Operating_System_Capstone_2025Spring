#include "vm.h"
#include "malloc.h"
#include "mmu.h"
#include "types.h"
#include "muart.h"
#include "mm.h"

#define LOG_VM 0 

/**
 * Walk the page table to find the PTE entry for a given virtual address
 * Returns pointer to the PTE entry
 */
unsigned long* walk(unsigned long* pagetable, unsigned long va) {
    // Start from PGD (level 0)
    unsigned long* current_table = pagetable;

    // Debug
    #if LOG_VM
    muart_puts("Walk pagetable VA: ");
    muart_send_hex((unsigned long)pagetable);
    muart_puts(", PA: ");
    muart_send_hex(VIRT_TO_PHYS(pagetable));
    muart_puts("\r\n");
    #endif

    // Walk through level 0 PGD -> level 1 PUD -> level 2 PMD
    for (int level = 0; level < 3; level++) {
        unsigned long idx;
        
        // Get index for current level
        switch (level) {
            case 0: idx = PGD_IDX(va); break;  // PGD level
            case 1: idx = PUD_IDX(va); break;  // PUD level  
            case 2: idx = PMD_IDX(va); break;  // PMD level
            default: return NULL;
        }
        
        unsigned long* pte = &current_table[idx];   
        // for level 0, *pte is PGD entry, it points to PUD table
        // for level 1, *pte is PUD entry, it points to PMD table
        // for level 2, *pte is PMD entry, it points to PTE table
        // for level 3, *pte is PTE entry, it points to physical page frame
    
        #if LOG_VM
        muart_puts("Level ");
        muart_send_dec(level);
        muart_puts(", idx: ");
        muart_send_hex(idx);
        muart_puts(", PTE: ");
        muart_send_hex(*pte);
        muart_puts("\r\n");
        #endif
        
        // If Bits[0] already set, it means the entry is valid, 代表這個 entry 有指向 page table 或是 page frame
        if (*pte & PD_VALID) {
            unsigned long next_table_pa = *pte & PHY_ADDR_MASK;  // Get the physical address Bits[47:12] of this page table entry (clear lower 12 bits for paged aligned)
            current_table = (unsigned long*)PHYS_TO_VIRT(next_table_pa);  // Translate to  kernel VA
            
            #if LOG_VM
            muart_puts("Next table PA: ");
            muart_send_hex(next_table_pa);
            muart_puts(", VA: ");
            muart_send_hex((unsigned long)current_table);
            muart_puts("\r\n");
            #endif
        } else { 
            // Allocate new page table for next level
            unsigned long* new_table = (unsigned long*)dmalloc(PAGE_SIZE);
            if (!new_table) {
                return NULL;
            }
            memzero((unsigned long)new_table, PAGE_SIZE);  // Initialize new page table to zero
            
            // Create page table entry pointing to new page table
            unsigned long new_table_pa = VIRT_TO_PHYS(new_table);
            *pte = new_table_pa | USER_TABLE_ATTR;  // Combine physical address with attributes
            current_table = new_table;

            #if LOG_VM
            muart_puts("Allocated new table VA: ");
            muart_send_hex((unsigned long)new_table);
            muart_puts(", PA: ");
            muart_send_hex(new_table_pa);
            muart_puts("\r\n");
            #endif
        }
    }
    
    // At PTE level, retrun PTE page table entry 的 pointer
    unsigned long pte_idx = PTE_IDX(va);
    #if LOG_VM
    muart_puts("Final PTE index: ");
    muart_send_hex(pte_idx);
    muart_puts("\r\n");
    #endif
    return &current_table[pte_idx];
}

/**
 * Map a range of virtual addresses to physical addresses
 * 
 * @param pagetable: PGD page table pointer for each task
 * @param va: Starting virtual address
 * @param size: Size to map
 * @param pa: Starting physical address
 * @return: 0 on success, -1 on failure
 */
int mappages(unsigned long* pagetable, unsigned long va, unsigned long size, unsigned long pa) {
    if (!pagetable) {
        return -1;
    }
    
    // Aligned the size to page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);  // Round up to nearest page size

    unsigned long end_va = va + size;
    unsigned long current_va = va;
    unsigned long current_pa = pa;
    
    // Map each page in the range
    while (current_va < end_va) {
        // Walk the page table to find the PTE for a given virtual address,  allocate if no page table exists
        unsigned long* pte = walk(pagetable, current_va);
        if (!pte) { // pte not found
            return -1;  // Failed to get PTE
        }
        
        // Create PTE entry, combine physical address with attributes
        *pte = current_pa | USER_PTE_ATTR;
        
        current_va += PAGE_SIZE;
        current_pa += PAGE_SIZE;
    }
    
    return 0;  // Success
}

/**
 * Recursively clear page table entries and free memory
 * @param pagetable: Page table pointer
 * @param level: Current level (0=PGD, 1=PUD, 2=PMD, 3=PTE)
 * @return: 0 on success, -1 on failure
 */
static int clear_pagetable_recursive(unsigned long* pagetable, int level) {
    if (!pagetable || level > 3) {
        return -1;
    }
    
    #if LOG_VM
    muart_puts("Clearing level ");
    muart_send_dec(level);
    muart_puts(" table at VA: ");
    muart_send_hex((unsigned long)pagetable);
    muart_puts("\r\n");
    #endif
    
    // Process all entries in the current table
    for (int i = 0; i < 512; i++) {
        unsigned long* pte = &pagetable[i];
        
        // Skip invalid entries
        if (!(*pte & PD_VALID)) {
            continue;
        }
        
        if (level < 3) {
            // Not at PTE level - recurse to next level
            unsigned long next_table_pa = *pte & PHY_ADDR_MASK;
            unsigned long* next_table = (unsigned long*)PHYS_TO_VIRT(next_table_pa);
            
            #if LOG_VM
            muart_puts("Processing entry[");
            muart_send_dec(i);
            muart_puts("] -> next table PA: ");
            muart_send_hex(next_table_pa);
            muart_puts(", VA: ");
            muart_send_hex((unsigned long)next_table);
            muart_puts("\r\n");
            #endif
            
            // Recursively clear the next level
            clear_pagetable_recursive(next_table, level + 1);
            
            // Free the next level table
            #if LOG_VM
            muart_puts("Freeing table at VA: ");
            muart_send_hex((unsigned long)next_table);
            muart_puts("\r\n");
            #endif
            dfree((void*)next_table);
            
        } else {
            // At PTE level - free physical pages
            unsigned long pa = *pte & PHY_ADDR_MASK;
            void* va = (void*)PHYS_TO_VIRT(pa);
            
            #if LOG_VM
            muart_puts("Freeing physical page[");
            muart_send_dec(i);
            muart_puts("] PA: ");
            muart_send_hex(pa);
            muart_puts(", VA: ");
            muart_send_hex((unsigned long)va);
            muart_puts("\r\n");
            #endif
            
            dfree(va);  // Free the physical page
        }
        
        // Clear the entry
        *pte = 0;
    }
    
    return 0;  // Success
}

/**
 * Clear all page tables corresponding to the specific PGD
 * Recursively frees all page tables and physical pages
 * @param pagetable: PGD page table pointer
 * @return: 0 on success, -1 on failure
 */
int clear_pagetable(unsigned long* pagetable) {
    if (!pagetable) {
        return -1;
    }
    
    #if LOG_VM
    muart_puts("Starting pagetable clear at PGD VA: ");
    muart_send_hex((unsigned long)pagetable);
    muart_puts(", PA: ");
    muart_send_hex(VIRT_TO_PHYS(pagetable));
    muart_puts("\r\n");
    #endif
    
    // Start recursive clearing from PGD level (level 0)
    int result = clear_pagetable_recursive(pagetable, 0);

    return result;
}