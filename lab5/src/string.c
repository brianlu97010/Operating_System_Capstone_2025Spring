#include "string.h"
#include "muart.h"
#include "types.h"

/**
 * Compares two C strings.
 * 
 * @return <0 if str1 < str2; 0 if equal; >0 if str1 > str2
 * @ref Based on GNU C library implementation (https://github.com/lattera/glibc/blob/master/string/strcmp.c)
 */int strcmp(const char* str1 , const char* str2){
    const unsigned char* u_str1 = (const unsigned char*)str1;
    const unsigned char* u_str2 = (const unsigned char*)str2;
    unsigned char c1, c2;

    do{
        c1 = *u_str1++;
        c2 = *u_str2++;
        if(!c1){
            return c1 - c2;  
        }
    }while( c1 == c2 );
    
    return c1 - c2;
}

/**
 * Converts a string to an integer
 * 
 * @param str The string to convert
 * @return The converted integer value
 * @ref Based on https://www.geeksforgeeks.org/c-atoi-function/
 */
int atoi(const char* str){
    int result = 0;
    while(*str){
        result = 10*result + (*str - '0');
        str++;
    }
    return result;
}

/**
 * Calculates the length of a string.
 * 
 * @param str The string to measure
 * @return The number of characters in the string, not including the null terminator
 */
size_t strlen(const char* str) {
    const char* s = str;
    
    while(*s) {
        s++;
    }
    
    return s - str;
}

/**
 * Copy a specified number of characters from one string to another.
 * 
 * @param dest: A pointer to the destination array where the content is to be copied; src: A pointer to the source string to be copied; n: The number of characters to be copied from the source string.
 * @return A pointer to the destination string dest
 */
char* strncpy(char* dest, const char* src, size_t n){
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/**
 * Compares at most the first n bytes of str1 and str2.
 * 
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @param n Maximum number of characters to compare
 * @return <0 if str1 < str2; 0 if equal; >0 if str1 > str2
 */
int strncmp(const char* str1, const char* str2, size_t n) {
    while (n-- > 0) {
        if (*str1 != *str2) {
            return (unsigned char)*str1 - (unsigned char)*str2;
        }
        if (*str1 == '\0') {
            return 0;
        }
        str1++;
        str2++;
    }
    return 0;
}