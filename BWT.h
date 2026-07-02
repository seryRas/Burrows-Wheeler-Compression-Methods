#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    succes = 0,
    mallocErr,
    emptyInput,

} errors;

typedef struct {
    unsigned char* data;
    size_t size;
    size_t initialIndex;
} bwt_out;

errors bwtTransform(unsigned char* input, bwt_out* output);

errors bwtRetransform(bwt_out* input, unsigned char* output);
