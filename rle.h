#ifndef RLE_H
#define RLE_H

#include <stdbool.h>

#include "errors.h"

#define SEQ_MAXLEN 4

typedef struct {
    unsigned char* next;
    unsigned char remainingToAdd;
    unsigned char lastChar;
    unsigned char charCount;
    bool done;
    unsigned int remainingSize;
} RLE1info;

errors RLE1encode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize,
                  unsigned char* inUseMap);

void RLE1infoInit(RLE1info* data, unsigned char* input, unsigned int inputSize);

errors RLE1decode(RLE1info* data, unsigned char** output,
                  unsigned int* outputSize, unsigned int chunkSize);

errors RLE2encode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize);

errors RLE2decode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize,
                  unsigned int expectedOutputSize);

#endif
