#include <stdio.h>
#include <stdint.h>

int main()
{
    char a[] = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde};
    uint16_t *c = (uint16_t *)a;
    printf("%p\n", a);
    for(int i = 0; i < 4; i++)
    {
        uint16_t d = *(c++);
        printf("%x\n", d);
    }
    uint16_t *b = (uint16_t *)(&a[2]);
    printf("%x\n", *b);
}