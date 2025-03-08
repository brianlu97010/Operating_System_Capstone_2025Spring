#ifndef _REGISTERS_H
#define _REGISTERS_H

/* Base physical address of peripherals */
#define MMIO_BASE               0x3F000000

/* GPIO registers */ 
#define GPFSEL1                 (MMIO_BASE+0x00200004)
#define GPPUD                   (MMIO_BASE+0x00200094)
#define GPPUDCLK0               (MMIO_BASE+0x00200098)

/* Mini UART registers */ 
#define AUXENB                  (MMIO_BASE+0x00215004)
#define AUX_MU_CNTL_REG         (MMIO_BASE+0x00215060)
#define AUX_MU_IER_REG          (MMIO_BASE+0x00215044)
#define AUX_MU_LCR_REG          (MMIO_BASE+0x0021504C)
#define AUX_MU_MCR_REG          (MMIO_BASE+0x00215050)
#define AUX_MU_BAUD             (MMIO_BASE+0x00215068)
#define AUX_MU_IIR_REG          (MMIO_BASE+0x00215048)
#define AUX_MU_LSR_REG          (MMIO_BASE+0x00215054)
#define AUX_MU_IO_REG           (MMIO_BASE+0x00215040)

/* Mailbox registers */
#define MAILBOX_BASE            (MMIO_BASE+0xB880)

#define MAILBOX_READ            (MAILBOX_BASE)      // Mailbox 0 Read/Write register
#define MAILBOX_STATUS          (MAILBOX_BASE+0x18) // Mailbox 0 status register
#define MAILBOX_WRITE           (MAILBOX_BASE+0x20) // Mailbox 1 Read/Write register

// Flags of status registers
#define MAILBOX_EMPTY           0x40000000
#define MAILBOX_FULL            0x80000000

/* Mailbox tags */
#define GET_BOARD_REVISION      0x00010002
#define GET_ARM_MEMORY          0x00010005
#define REQUEST_CODE            0x00000000
#define REQUEST_SUCCEED         0x80000000
#define REQUEST_FAILED          0x80000001
#define TAG_REQUEST_CODE        0x00000000
#define END_TAG                 0x00000000

/* Power management */
#define PM_BASE		            (MMIO_BASE + 0x100000)  // 0x3F100000
#define PM_RSTC		            (PM_BASE + 0x1C)        // ReSeT Control register
#define PM_RSTS		            (PM_BASE + 0x20)        // ReSeT Status register
#define PM_WDOG		            (PM_BASE + 0x24)        // WatchDOG timer register
#define PM_PASSWD		        (0x5A << 24)            // 0x5a000000, any writing operation to WDT register needs this password
#define PM_RSTC_CLEAR	        0xFFFFFFCF
#define PM_RSTC_REBOOT	        0x00000020              // full reset

#endif