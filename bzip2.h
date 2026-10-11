#ifndef BZIP2_H
#define BZIP2_H

#include "BWT.h"
#include "rle.h"
#include "mtf.h"
#include "huffmanEncoding.h"

#define IN_USE_MAP_SIZE 32

typedef struct {
    unsigned int crc32;                       // Checksum
    unsigned int bwtIndex;                    // BWT initial index
    unsigned int origSize;                    // Original block size
    unsigned int rle1Size;                    // Size after RLE1 (BWT/MTF size)
    unsigned int rle2Size;                    // Size after RLE2 (Huffman uncompressed size)
    unsigned int compressedSize;              // Size of Huffman payload
    unsigned char inUseMap[IN_USE_MAP_SIZE];  // alphabet thats used in data
} bzip2Header;

errors bzip2CompressBlock(unsigned char* inputData, unsigned int dataSize,
                          unsigned char** outputData, unsigned int* outputSize);

errors bzip2DecompressBlock(unsigned char* inputData, unsigned int inputSize,
                            unsigned char** outputData, unsigned int* outputSize);

#endif
