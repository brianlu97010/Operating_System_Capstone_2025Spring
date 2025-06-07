#ifndef _MMU_H
#define _MMU_H

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
#define PD_TABLE                         0b11
#define PD_BLOCK                         0b01
#define PD_ACCESS                       (1 << 10)
#define BOOT_PGD_ATTR                   PD_TABLE
#define BOOT_PUD_ATTR                   (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#endif