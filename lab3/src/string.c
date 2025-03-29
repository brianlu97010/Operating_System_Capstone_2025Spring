#include "string.h"
#include "muart.h"
#include "types.h"

int strcmp(const char* str1 , const char* str2){
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

int atoi(const char* str){
    int result = 0;
    while(*str){
        result = 10*result + (*str - '0');
        str++;
    }
    return result;
}

size_t strlen(const char* str) {
    const char* s = str;
    
    while(*s) {
        s++;
    }
    
    return s - str;
}

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