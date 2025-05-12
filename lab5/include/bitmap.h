#ifndef _BITMAP_H
#define _BITMAP_H

#define BITS_PER_LONG               (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(nr)           (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)    // size of bitmap
#define DECLARE_BITMAP(name, bits)  unsigned long name[BITS_TO_LONGS(bits)]

#define BIT_MASK(nr)                (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)                ((nr) / BITS_PER_LONG)

#define test_bit(nr, addr)    (BIT_MASK(nr) & (((unsigned long *)addr)[BIT_WORD(nr)])) != 0
#define set_bit(nr, addr)     ((unsigned long *)addr)[BIT_WORD(nr)] |= (BIT_MASK(nr))
#define clear_bit(nr, addr)   (((unsigned long *)addr)[BIT_WORD(nr)] &= ~(BIT_MASK(nr)))

#endif