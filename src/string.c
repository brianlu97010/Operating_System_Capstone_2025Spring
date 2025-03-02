#include "string.h"
#include "muart.h"

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
