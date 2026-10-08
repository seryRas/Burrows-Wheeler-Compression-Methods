#ifndef BZIP2_H
#define BZIP2_H

#include "BWT.h"

#include "rle.h"
#include "mtf.h"

#define BZIP2_MAGIC "\x31\x41\x59\x26\x53\x59"  // Pi digits block magic
#define BZIP2_MAGIC_LEN 6
#define IN_USE_MAP_SIZE 32

typedef struct {
    unsigned char magic[BZIP2_MAGIC_LEN];     // Block magic bytes
    unsigned int crc32;                       // Checksum
    unsigned int bwtIndex;                    // BWT initial index
    unsigned char inUseMap[IN_USE_MAP_SIZE];  // alphabet thats used in data
} bzip2Header;

#endif
