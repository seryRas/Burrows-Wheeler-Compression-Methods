#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_TYPE true
#define L_TYPE false
#define BEGIN 0
#define END 1
#define AMOUNT_OF_VALUES 0x100
typedef enum {
    success = 0,
    mallocErr,
    emptyInput,
    fileErr,
    generalError,

} errors;

typedef struct {
    unsigned char* data;
    size_t size;
    size_t initialIndex;
} bwt_out;

typedef struct {
    size_t* data;
    size_t size;
    size_t initialIndex;
} rec_sais_out;

typedef struct {
    size_t indexAmount;
    size_t* array;
} LMSArray;

errors bwtTransform(unsigned char* input, bwt_out* output);

errors bwtRetransform(bwt_out* input, unsigned char* output);
