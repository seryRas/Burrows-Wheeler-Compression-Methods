#ifndef BZIP2_H
#define BZIP2_H

#include "BWT.h"
#include "rle.h"
#include "mtf.h"
#include "huffmanEncoding.h"

#define IN_USE_MAP_SIZE 32

typedef struct {
    unsigned int bwtIndex;                    // BWT initial index
    unsigned int origSize;                    // Original block size
    unsigned int rle1Size;                    // Size after RLE1 (BWT/MTF size)
    unsigned int rle2Size;                    // Size after RLE2 (Huffman uncompressed size)
    unsigned int compressedSize;              // Size of Huffman payload
    unsigned char inUseMap[IN_USE_MAP_SIZE];  // alphabet thats used in data
} bzip2Header;

typedef struct {
    double rle1Time;
    double bwtTime;
    double mtfTime;
    double rle2Time;
    double huffmanTime;
} bzip2StepTiming;

errors bzip2CompressBlockTimed(unsigned char* inputData, unsigned int dataSize,
                               unsigned char** outputData, unsigned int* outputSize,
                               bzip2StepTiming* timing);

errors bzip2CompressBlock(unsigned char* inputData, unsigned int dataSize,
                          unsigned char** outputData, unsigned int* outputSize);

errors bzip2DecompressBlockTimed(unsigned char* inputData, unsigned int inputSize,
                                 unsigned char** outputData, unsigned int* outputSize,
                                 bzip2StepTiming* timing);

errors bzip2DecompressBlock(unsigned char* inputData, unsigned int inputSize,
                            unsigned char** outputData, unsigned int* outputSize);

#endif
