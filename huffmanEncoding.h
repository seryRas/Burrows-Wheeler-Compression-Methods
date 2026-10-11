#ifndef HUFFMAN_ENCODING_H
#define HUFFMAN_ENCODING_H

#include "errors.h"

#define ALPHABET_SIZE 256
#define HUFFMAN_HEADER ALPHABET_SIZE
#define MAX_DEPTH 15
#define MAX_NODE_AMOUNT 511  // 2 * ALPHABET_SIZE - 1
#define ROOT_PARENT __UINT16_MAX__

typedef struct {
    unsigned int amount;
    unsigned short parentIdx;
} HuffmanNode;

typedef struct {
    unsigned short code;
    unsigned char length;
} HuffmanMapping;

typedef struct {
    unsigned char symbol;
    unsigned char length;
} DecodeEntry;

errors huffmanEncode(unsigned int* frequencyMap, unsigned char* input,
                     unsigned char** output, unsigned int inputSize,
                     unsigned int* outputSize);

errors huffmanDecode(unsigned char* input, unsigned int inputSize,
                     unsigned char** output, unsigned int* outputSize,
                     unsigned int expectedOutputSize);

#endif
