#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include "hexd.h"

// Hexdump an array. out should be 2*len elements.
void hexdump(char* in, char* out, size_t len) {
    for(int i=0; i < len; i++) {
        out[(i*2)+1]   = ("0123456789abcdef")[ in[i] & 0x0F ];
        out[(i*2)]     = ("0123456789abcdef")[ (in[i] >> 4) & 0x0F ];
    }
}

// Unhexdump a hexadecimal string. Must be zero-terminated.
void unhexdump(char* in, char* out) {
    for(int i=0; i < strlen(in) / 2; i++) {
        unsigned char at0 = in[i*2];
        unsigned char at1 = in[i*2+1];

        if      (at0 >= 'a' && at0 <= 'f')
            at0 -= ('a' - 10);
        else if (at0 >= 'A' && at0 <= 'F')
            at0 -= ('A' - 10);
        else if (at0 >= '0' && at0 <= '9')
            at0 -= '0';

        if      (at1 >= 'a' && at1 <= 'f')
            at1 -= ('a' - 10);
        else if (at1 >= 'A' && at1 <= 'F')
            at1 -= ('A' - 10);
        else if (at1 >= '0' && at1 <= '9')
            at1 -= '0';

        out[i] = ((at0 & 0x0f) << 4) | (at1 & 0x0f);
    }
}

