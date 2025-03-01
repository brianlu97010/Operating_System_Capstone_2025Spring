#ifndef _STRING_H
#define _STRING_H

/**
 * Compares two C strings.
 * 
 * @return <0 if str1 < str2; 0 if equal; >0 if str1 > str2
 * @ref Based on GNU C library implementation (https://github.com/lattera/glibc/blob/master/string/strcmp.c)
*/
int strcmp(const char*, const char*);

/* Write the C string str to the transmit FIFO by mini UART */ 
void puts(const char*);

#endif