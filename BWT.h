#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define S_TYPE true
#define L_TYPE false
#define BEGIN 0
#define END 1
#define AMOUNT_OF_VALUES 0x100
#define BWT_HEADER_SIZE (1 + sizeof(unsigned int))
#define ALL_SAME_INPUT 3
#define EMPTY_IDX UINT_MAX
#define MAX_CHUNK 921600//4194304 // 4MB
typedef enum {
    success = 0,
    mallocErr,
    emptyInput,
    fileErr,
    generalError,
} errors;

typedef struct {
    unsigned int* data;
    unsigned int size;
    unsigned int initialIndex;
} rec_sais_out;

typedef struct {
    unsigned int indexAmount;
    unsigned int* array;
} LMSArray;

errors bwtTransform(unsigned char* input, unsigned int inputSize,
                    unsigned char* output);

errors bwtRetransform(unsigned char* input, unsigned int inputSize,
                      unsigned char* output);
