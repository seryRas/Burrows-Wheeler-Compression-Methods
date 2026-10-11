#ifndef BIT_OPERATORS_C
#define BIT_OPERATORS_C

#include <stdlib.h>

#include "errors.h"

#define BITS_IN_BYTE 8

typedef struct {
    unsigned int leftoverBits;
    unsigned char bitAmount;
    size_t outputIdx;
} writerData;

typedef struct {
    unsigned int incomingBits;
    unsigned char bitAmount;
    size_t inputIdx;
    size_t inputSize;
} readerData;

static inline void writeData(unsigned char size, unsigned int value,
                             unsigned char* output, writerData* data) {
    data->leftoverBits |=
        value << ((sizeof(unsigned int) * BITS_IN_BYTE) - data->bitAmount - size);
    data->bitAmount += size;
    while (data->bitAmount >= sizeof(unsigned char) * BITS_IN_BYTE) {
        output[data->outputIdx++] =
            (unsigned char)(data->leftoverBits >>
                            (sizeof(unsigned int) - sizeof(unsigned char)) *
                                BITS_IN_BYTE);
        data->leftoverBits = data->leftoverBits
                             << sizeof(unsigned char) * BITS_IN_BYTE;
        data->bitAmount -= sizeof(unsigned char) * BITS_IN_BYTE;
    }
}

static inline void readerFillData(unsigned char* input, readerData* data) {
    while (data->bitAmount <=
           (sizeof(unsigned int) - sizeof(unsigned char)) * BITS_IN_BYTE) {
        unsigned char byte = 0;
        if (data->inputSize == 0 || data->inputIdx < data->inputSize) {
            byte = input[data->inputIdx++];
        }
        data->incomingBits |=
            ((unsigned int)byte)
            << (((sizeof(unsigned int) - sizeof(unsigned char)) * BITS_IN_BYTE) -
                   data->bitAmount);
        data->bitAmount += sizeof(unsigned char) * BITS_IN_BYTE;
    }
}

static inline unsigned short readerGetBits(unsigned char amount,
                                           readerData* data,
                                           unsigned char* input) {
    if (data->bitAmount < amount) readerFillData(input, data);
    return (unsigned short)(data->incomingBits >>
                            ((sizeof(unsigned int) * BITS_IN_BYTE) - amount));
}

static inline void readerMoveBits(unsigned char amount, readerData* data) {
    data->incomingBits = data->incomingBits << amount;
    data->bitAmount -= amount;
}

#endif