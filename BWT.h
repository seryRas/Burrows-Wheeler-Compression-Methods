#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char* data;
    size_t size;
    size_t index;
} bwt_out;

unsigned short bwtTransform(unsigned char* input, bwt_out* output);
