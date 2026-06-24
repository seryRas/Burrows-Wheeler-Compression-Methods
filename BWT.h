#include <stdio.h>



typedef struct {
    unsigned char *data;
    size_t size;
    size_t index;
} bwt_out;

typedef struct {
    size_t currentLen;
    size_t indexArray[sizeof(size_t)];
} sortingArray;

unsigned short bwtTransform(unsigned char* input, bwt_out* output);
