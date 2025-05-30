#ifndef _TYPES_H
#define _TYPES_H

#ifndef __ASSEMBLER__
/* Size types */
typedef unsigned long size_t;    // For sizes and indices, on a 64-bit system size_t will take 64 bits

typedef unsigned long pid_t;
#endif 

/* NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif