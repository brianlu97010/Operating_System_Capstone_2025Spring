#ifndef _STRING_H
#define _STRING_H

/**
 * Compares two C strings.
 * 
 * @return <0 if str1 < str2; 0 if equal; >0 if str1 > str2
 * @ref Based on GNU C library implementation (https://github.com/lattera/glibc/blob/master/string/strcmp.c)
*/
int strcmp(const char*, const char*);


/**
 * Converts a string to an integer
 * 
 * @param str The string to convert
 * @return The converted integer value
 * @ref Based on https://www.geeksforgeeks.org/c-atoi-function/
 */
int atoi(const char* str);

#endif