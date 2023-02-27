#include "util.h"
#include <iostream>

namespace Util
{

void dumpHex(unsigned char* data, int len)
{
    for(int i = 0; i < len; i++) {
        if(i % 16 == 0 && i != 0)
            printf("\n");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

}