#ifndef _MMU_H
#define _MMU_H

/* Translate the kernel virtual address to physical address */
#define KERNEL_VA_BASE    0xFFFF000000000000
#define VIRT_TO_PHYS(va)  ((unsigned long)(va) - KERNEL_VA_BASE)
#define PHYS_TO_VIRT(pa)  ((unsigned long)(pa) + KERNEL_VA_BASE)

// First 1GB: RAM and GPU peripherals
// Physical addresses range from 0x3C000000 to 0x3CFFFFFF for peripherals (Ref : BCM2837 ARM Peripherals Manual)
#define RAM_START                       0x00000000
#define RAM_END                         0x3C000000
#define PERIPHERAL_START                0x3C000000    // It will be access in user program
#define PERIPHERAL_END                  0x40000000

#define BLOCK_SIZE_2MB                  0x200000


/* 
 *********** Translation Control Register (TCR) Configuration ***********
 * T1SZ, `bits [21:16]` = `10000`
    * The size offset of the memory region addressed by `TTBR1_EL1`. (addressable region for kerenl space)
    * The region size is $2^{64-T1SZ} = 2^{48}$ bytes.
 * T0SZ, `bits [5:0]` 設為 `10000`
    * The size offset of the memory region addressed by `TTBR0_EL1`. (addressable region for user space)
    * The region size is $2^{64-T0SZ} = 2^{48}$ bytes. 
 * TG0,`bits [15:14]` 設為 `00`
    * Granule size for the corresponding translation table base address register.
    * `00` 4KByte
 * TG1, `bits [31:30]` 設為 `10`
    * TTBR1_EL1 Granule size.
    * `10` 4KByte
 */
#define TCR_CONFIG_REGION_48bit         (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB                  ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT              (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/*
 *********** Memory Attribute Indirection Register (MAIR) Configuration ***********
 * Device memory nGnRnE:
    * `Attr<0>[7:4] = 0000 Attr<0>[3:0] = 0000`
    * Peripheral access.
 * Normal memory without cache:
    * `Attr<1>[7:4] = 0100 Attr<0>[3:0] = 0100`
 */
#define MAIR_DEVICE_nGnRnE               0b00000000
#define MAIR_NORMAL_NOCACHE              0b01000100
#define MAIR_IDX_DEVICE_nGnRnE                    0
#define MAIR_IDX_NORMAL_NOCACHE                   1
#define MAIR_VALUE                       ( (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
                                           (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) )

/* 
 *********** Flags of page descriptor ***********

 * In general : 
 * Bits[1:0] Specify the next level is a block/page, page table, or invalid. 
    `11` : point to a page table for PGD, PUD, PMD ; point to a page for PTE
    `01` : point to a block for PUD, PMD
    `00`, `10` : invalid entry 

 * Page / Block descriptor :
 * Bits[10] : Access Flag, if not set, it will generate a page fault
 * Bits[4:2] : The index to `MAIR` register. 
   `000` : Device memory nGnRnE (Map all memory as Device nGnRnE)

 * Table descriptor : 
 *  `Bits[47:12]` : the address of the required next-level table
 *  `Bits[11:0]` of the table address are zero.
 */
#define PD_VALID                         0b1
#define PD_TABLE                         0b11
#define PD_BLOCK                         0b01
#define PD_ACCESS                       (1 << 10)
#define PD_USER                         (1 << 6)   // 0 for only kernel access, 1 for user/kernel access.
#define PD_READONLY                     (1 << 7)   // 0 for read-write, 1 for read-only (Note that If you set Bits[7:6] to 0b01, which means the user can read/write the region, then the kernel is automatically not executable in that region no matter what the value of Bits[53] is.)

// Page Table Entry Attribute for kernel space 
#define BOOT_PGD_ATTR                   PD_TABLE
// #define BOOT_PUD_ATTR                   (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK) // this is block descriptor, for two level translation
#define BOOT_PUD_ATTR                   PD_TABLE
#define BOOT_PMD_ATTR_RAM              (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK) // this is block descriptor, for 0x00000000 ~ 0x3F000000 set to normal non-cache memory
#define BOOT_PMD_ATTR_PERIPHERAL       (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK) // this is block descriptor, for 0x3F000000 ~ 0x40000000 and 0x40000000 ~ 0x7FFFFFFFFF set to device nGnRnE memory 

// Page Table Entry Attribute for user spce
#define USER_TABLE_ATTR                 PD_TABLE
#define USER_PTE_ATTR                  (PD_ACCESS | PD_USER | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE)

#endif