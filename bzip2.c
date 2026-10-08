#include "bzip2.h"



#ifndef BZIP2_TEST

static int cleanupBlock(errors status, unsigned char* rle1Out,
                        unsigned char* bwtOut, unsigned char* mtfOut,
                        unsigned char* rle2Out) {
    if (status != success)
        fprintf(stderr, "Compression block failed: %d\n", status);
    free(rle1Out);
    free(bwtOut);
    free(mtfOut);
    free(rle2Out);
    return status;
}

static int processBlock(unsigned char* inputData, unsigned int dataSize,
                        int blockNum) {
    bzip2Header header;
    memcpy(header.magic, BZIP2_MAGIC, BZIP2_MAGIC_LEN);
    header.crc32 = 0;  // TODO: Implement CRC32 later
    header.bwtIndex = 0;
    memset(header.inUseMap, 0, IN_USE_MAP_SIZE);

    errors status;
    unsigned char* rle1Out = NULL;
    unsigned char* bwtOut = NULL;
    unsigned char* mtfOut = NULL;
    unsigned char* rle2Out = NULL;
    unsigned int rle1Size = 0;
    unsigned int rle2Size = 0;

    printf("--- Processing Block %d (%u bytes) ---\n", blockNum, dataSize);

    // 1. RLE 1
    status =
        RLE1encode(inputData, dataSize, &rle1Out, &rle1Size, header.inUseMap);
    if (status != success)
        return cleanupBlock(status, rle1Out, bwtOut, mtfOut, rle2Out);

    // 2. BWT
    bwtOut = malloc(rle1Size);
    if (!bwtOut)
        return cleanupBlock(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out);

    status = bwtTransform(rle1Out, rle1Size, bwtOut, &header.bwtIndex);
    free(rle1Out);
    rle1Out = NULL;
    if (status != success)
        return cleanupBlock(status, rle1Out, bwtOut, mtfOut, rle2Out);

    // 3. MTF
    unsigned char mtfDict[AMOUNT_OF_VALUES];
    unsigned int dictSize = 0;
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
        if (header.inUseMap[i / 8] & (1 << (i % 8))) {
            mtfDict[dictSize++] = (unsigned char)i;
        }
    }

    mtfOut = malloc(rle1Size);
    if (!mtfOut)
        return cleanupBlock(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out);

    status = MTFencode(bwtOut, rle1Size, mtfOut, mtfDict, dictSize);
    free(bwtOut);
    bwtOut = NULL;
    if (status != success)
        return cleanupBlock(status, rle1Out, bwtOut, mtfOut, rle2Out);

    // 4. RLE 2
    status = RLE2encode(mtfOut, rle1Size, &rle2Out, &rle2Size);
    if (status != success)
        return cleanupBlock(status, rle1Out, bwtOut, mtfOut, rle2Out);

    // Summary
    printf("Block %d completed!\n", blockNum);
    printf("  Original Size: %u bytes\n", dataSize);
    printf("  Final Data Size (before Huffman): %u bytes\n", rle2Size);
    printf("  BWT Initial Index stored in header: %u\n", header.bwtIndex);

    return cleanupBlock(success, rle1Out, bwtOut, mtfOut, rle2Out);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path> [block_size_kb]\n", argv[0]);
        return generalError;
    }

    // Default block size is 900KB
    unsigned int blockSize = 900 * 1024;

    if (argc >= 3) {
        char* endptr;
        long long kb = strtoll(argv[2], &endptr, 10);
        
        if (endptr == argv[2] || *endptr != '\0') {
            fprintf(stderr, "Invalid block size. Must be a valid number.\n");
            return generalError;
        }

        if (kb <= 0) {
            fprintf(stderr, "Invalid block size. Must be > 0.\n");
            return generalError;
        }

        if (kb > (long long)(UINT_MAX / 1024)) {
            fprintf(stderr, "Block size too large. Maximum is %u KB.\n", UINT_MAX / 1024);
            return sizeTooBig;
        }

        blockSize = (unsigned int)(kb * 1024);
    }

    FILE* inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open input file\n");
        return fileErr;
    }

    // Allocate buffer for reading chunks
    unsigned char* inputData = malloc(blockSize);
    if (!inputData) {
        fclose(inputFile);
        return mallocErr;
    }

    printf("Starting compression pipeline with block size: %u bytes\n",
           blockSize);

    int blockNum = 1;
    size_t bytesRead;

    while ((bytesRead = fread(inputData, 1, blockSize, inputFile)) > 0) {
        unsigned int currentSize = (unsigned int)bytesRead;
        int status = processBlock(inputData, currentSize, blockNum);
        if (status != success) {
            free(inputData);
            fclose(inputFile);
            return status;
        }
        blockNum++;
    }

    // Check for read errors
    if (ferror(inputFile)) {
        fprintf(stderr, "Error reading from file\n");
        free(inputData);
        fclose(inputFile);
        return fileErr;
    }

    free(inputData);
    fclose(inputFile);

    if (blockNum == 1) {
        printf("File was empty. No blocks processed.\n");
    } else {
        printf("All blocks processed successfully.\n");
    }

    return success;
}
#endif
