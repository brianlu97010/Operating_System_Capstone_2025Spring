#ifndef _STRING_H
#define _STRING_H

#include "types.h"

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

/**
 * Calculates the length of a string.
 * 
 * @param str The string to measure
 * @return The number of characters in the string, not including the null terminator
 */
size_t strlen(const char* str);

/**
 * Copy a specified number of characters from one string to another.
 * 
 * @param dest: A pointer to the destination array where the content is to be copied; src: A pointer to the source string to be copied; n: The number of characters to be copied from the source string.
 * @return A pointer to the destination string dest
 */
char* strncpy(char* dest, const char* src, size_t n);

/**
 * strcpy - Copy string from source to destination
 * @dest: destination buffer
 * @src: source string
 * 
 * Returns: pointer to destination string
 * 
 * Note: Caller must ensure dest has enough space to hold src + null terminator
 */
char* strcpy(char* dest, const char* src);

/**
 * strcat - Concatenate source string to destination string
 * @dest: destination string (must be null-terminated)
 * @src: source string to append
 * 
 * Returns: pointer to destination string
 * 
 * Note: Caller must ensure dest has enough space to hold the concatenated result
 */
char* strcat(char* dest, const char* src);

#endif