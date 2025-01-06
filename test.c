#include <stdio.h>

typedef __uint8_t u8 ;
int main(){
    u8 n = 229; //
    u8 k = 3;  //0011
    //suppose left , and expecting 8bit
    u8 newN = (n >> 5) & 0b111;
    n = (n << 3) | newN;
    printf("%d", n);

    return 0;
}