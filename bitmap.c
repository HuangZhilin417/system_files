#include <stdint.h>
#include <stdio.h>
#include "bitmap.h"

int
bitmap_get(void* bm, int ii) {
    return ((uint8_t*)bm)[ii / 8] & (1 << (ii & 7)) ? 1 : 0;
}

void
bitmap_put(void* bm, int ii, int vv) {
    if (vv == 0) {
        ((uint8_t*)bm)[ii / 8] &= ~(1 << (ii & 7));
    }
    else {
        ((uint8_t*)bm)[ii / 8] |= 1 << (ii & 7);
    }
} 

void
bitmap_print(void* bm, int size) {
    for (int i = 0; i < size; ++i) {
        uint8_t b = ((uint8_t*)bm)[i];

        for (int j = 0; j < 8; ++j) {
            printf("%d", !!((b << j) & 0x80));
        }
        printf("\n");
    }
}
