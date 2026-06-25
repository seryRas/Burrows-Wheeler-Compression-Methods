#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    unsigned char *data;
    size_t size;
    size_t index;
} bwt_out;

typedef struct {
    size_t currentLen;
    size_t *indexArray;
} sortingArray;

unsigned short bwtTransform(unsigned char* input, bwt_out* output);
