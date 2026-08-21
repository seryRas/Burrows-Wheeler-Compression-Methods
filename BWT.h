#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_TYPE true
#define L_TYPE false
#define BEGIN 0
#define END 1
#define AMOUNT_OF_VALUES 0x100
#define BWT_HEADER_SIZE (1 + sizeof(size_t))
#define BWT_METHOD_SAIS 0x01
#define BWT_METHOD_RADIX 0x02
#define ALL_SAME_INPUT 3
typedef enum {
    success = 0,
    mallocErr,
    emptyInput,
    fileErr,
    generalError,
} errors;

typedef struct {
    size_t* data;
    size_t size;
    size_t initialIndex;
} rec_sais_out;

typedef struct {
    size_t indexAmount;
    size_t* array;
} LMSArray;

errors bwtTransform(unsigned char* input, size_t inputSize,
                    unsigned char* output);

errors bwtRetransform(unsigned char* input, size_t inputSize,
                      unsigned char* output);
