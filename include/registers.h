#ifndef _REGISTERS_H
#define _REGISTERS_H

/* Base physical address of peripherals */
#define PHYADDR                                                  0x3F000000

/* GPIO registers */ 
#define GPFSEL1                 (volatile unsigned int*)(PHYADDR+0x00200004)
#define GPPUD                   (volatile unsigned int*)(PHYADDR+0x00200094)
#define GPPUDCLK0               (volatile unsigned int*)(PHYADDR+0x00200098)

/* Mini UART registers */ 
#define AUXENB                  (volatile unsigned int*)(PHYADDR+0x00215004)
#define AUX_MU_CNTL_REG         (volatile unsigned int*)(PHYADDR+0x00215060)
#define AUX_MU_IER_REG          (volatile unsigned int*)(PHYADDR+0x00215044)
#define AUX_MU_LCR_REG          (volatile unsigned int*)(PHYADDR+0x0021504C)
#define AUX_MU_MCR_REG          (volatile unsigned int*)(PHYADDR+0x00215050)
#define AUX_MU_BAUD             (volatile unsigned int*)(PHYADDR+0x00215068)
#define AUX_MU_IIR_REG          (volatile unsigned int*)(PHYADDR+0x00215048)
#define AUX_MU_LSR_REG          (volatile unsigned int*)(PHYADDR+0x00215054)
#define AUX_MU_IO_REG           (volatile unsigned int*)(PHYADDR+0x00215040)

#endif