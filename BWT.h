#ifndef BWT_H
#define BWT_H

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

#define S_TYPE true
#define L_TYPE false
#define BEGIN 0
#define END 1
#define AMOUNT_OF_VALUES 0x100
#define BWT_HEADER_SIZE (1 + sizeof(unsigned int))
#define ALL_SAME_INPUT 3
#define EMPTY_IDX UINT_MAX

typedef struct {
    unsigned int* data;
    unsigned int size;
    unsigned int initialIndex;
} RecSaisOut;

typedef struct {
    unsigned int indexAmount;
    unsigned int* array;
} LmsArray;

// bwtTransform applies the Burrows-Wheeler Transform to the input data
// `output` must be preallocated to atleast size of `inputSize`.
// function stores the index of original input in `bwtIndex`.
errors bwtTransform(unsigned char* input, unsigned int inputSize,
                    unsigned char* output, unsigned int* bwtIndex);

// bwtRetransform reverses the BWT
// `output` must be preallocated to atleast size of `inputSize`.
errors bwtRetransform(unsigned char* input, unsigned int inputSize,
                      unsigned char* output, unsigned int bwtIndex);

#endif
