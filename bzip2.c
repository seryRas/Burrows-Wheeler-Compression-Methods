#include "bzip2.h"

static inline errors freeCompressBuffers(errors status, unsigned char* rle1,
                                         unsigned char* bwt, unsigned char* mtf,
                                         unsigned char* rle2,
                                         unsigned char* huff) {
    free(rle1);
    free(bwt);
    free(mtf);
    free(rle2);
    free(huff);
    return status;
}

static inline errors freeDecompressBuffers(errors status, unsigned char* rle2,
                                           unsigned char* mtf,
                                           unsigned char* bwt,
                                           unsigned char* rle1) {
    free(rle2);
    free(mtf);
    free(bwt);
    free(rle1);
    return status;
}

errors bzip2CompressBlock(unsigned char* inputData, unsigned int dataSize,
                          unsigned char** outputData, unsigned int* outputSize) {
    if (dataSize == 0) {
        *outputData = NULL;
        *outputSize = 0;
        return success;
    }

    bzip2Header header;
    header.crc32 = 0;  // TODO: Implement CRC32 later
    header.bwtIndex = 0;
    header.origSize = dataSize;
    memset(header.inUseMap, 0, IN_USE_MAP_SIZE);

    errors status;
    unsigned char* rle1Out = NULL;
    unsigned char* bwtOut = NULL;
    unsigned char* mtfOut = NULL;
    unsigned char* rle2Out = NULL;
    unsigned char* huffOut = NULL;
    unsigned int rle1Size = 0;
    unsigned int rle2Size = 0;
    unsigned int huffSize = 0;

    // 1. RLE 1
    status =
        RLE1encode(inputData, dataSize, &rle1Out, &rle1Size, header.inUseMap);
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);
    header.rle1Size = rle1Size;

    // 2. BWT
    bwtOut = malloc(rle1Size);
    if (!bwtOut)
        return freeCompressBuffers(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    status = bwtTransform(rle1Out, rle1Size, bwtOut, &header.bwtIndex);
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

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
        return freeCompressBuffers(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    status = MTFencode(bwtOut, rle1Size, mtfOut, mtfDict, dictSize);
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    // 4. RLE 2
    unsigned int frequencyMap[ALPHABET_SIZE] = {0};
    status = RLE2encode(mtfOut, rle1Size, &rle2Out, &rle2Size, frequencyMap);
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);
    header.rle2Size = rle2Size;

    // 5. Huffman
    status =
        huffmanEncode(frequencyMap, rle2Out, &huffOut, rle2Size, &huffSize);
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);
    header.compressedSize = huffSize;

    // Assemble block: [bzip2Header] + [Huffman Payload]
    *outputSize = (unsigned int)sizeof(bzip2Header) + huffSize;
    *outputData = malloc(*outputSize);
    if (!(*outputData))
        return freeCompressBuffers(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    memcpy(*outputData, &header, sizeof(bzip2Header));
    memcpy(*outputData + sizeof(bzip2Header), huffOut, huffSize);

    return freeCompressBuffers(success, rle1Out, bwtOut, mtfOut, rle2Out,
                               huffOut);
}

errors bzip2DecompressBlock(unsigned char* inputData, unsigned int inputSize,
                            unsigned char** outputData,
                            unsigned int* outputSize) {
    *outputData = NULL;
    *outputSize = 0;

    if (inputSize < sizeof(bzip2Header)) return corruptedData;

    bzip2Header header;
    memcpy(&header, inputData, sizeof(bzip2Header));

    if (inputSize < sizeof(bzip2Header) + header.compressedSize)
        return corruptedData;

    if (header.origSize == 0) {
        return success;
    }

    errors status;
    unsigned char* huffIn = inputData + sizeof(bzip2Header);
    unsigned char* rle2Dec = NULL;
    unsigned char* mtfDec = NULL;
    unsigned char* bwtDec = NULL;
    unsigned char* rle1Dec = NULL;
    unsigned int rle2DecSize = 0;
    unsigned int mtfDecSize = 0;

    // 1. Huffman Decode
    status = huffmanDecode(huffIn, header.compressedSize, &rle2Dec,
                           &rle2DecSize, header.rle2Size);
    if (status != success || rle2DecSize != header.rle2Size) {
        return freeDecompressBuffers(status != success ? status : corruptedData,
                                     rle2Dec, mtfDec, bwtDec, rle1Dec);
    }

    // 2. RLE 2 Decode
    status =
        RLE2decode(rle2Dec, rle2DecSize, &mtfDec, &mtfDecSize, header.rle1Size);
    if (status != success || mtfDecSize != header.rle1Size) {
        return freeDecompressBuffers(status != success ? status : corruptedData,
                                     rle2Dec, mtfDec, bwtDec, rle1Dec);
    }

    // 3. MTF Decode
    unsigned char mtfDict[AMOUNT_OF_VALUES];
    unsigned int dictSize = 0;
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
        if (header.inUseMap[i / 8] & (1 << (i % 8))) {
            mtfDict[dictSize++] = (unsigned char)i;
        }
    }

    bwtDec = malloc(header.rle1Size);
    if (!bwtDec)
        return freeDecompressBuffers(mallocErr, rle2Dec, mtfDec, bwtDec,
                                     rle1Dec);

    status = MTFdecode(mtfDec, header.rle1Size, bwtDec, mtfDict, dictSize);
    if (status != success)
        return freeDecompressBuffers(status, rle2Dec, mtfDec, bwtDec, rle1Dec);

    // 4. BWT Retransform
    rle1Dec = malloc(header.rle1Size);
    if (!rle1Dec)
        return freeDecompressBuffers(mallocErr, rle2Dec, mtfDec, bwtDec,
                                     rle1Dec);

    status = bwtRetransform(bwtDec, header.rle1Size, rle1Dec, header.bwtIndex);
    if (status != success)
        return freeDecompressBuffers(status, rle2Dec, mtfDec, bwtDec, rle1Dec);

    // 5. RLE 1 Decode
    RLE1info rle1Info;
    RLE1infoInit(&rle1Info, rle1Dec, header.rle1Size);
    status = RLE1decode(&rle1Info, outputData, outputSize, header.origSize);
    if (status != success || *outputSize != header.origSize) {
        if (*outputData) {
            free(*outputData);
            *outputData = NULL;
        }
        *outputSize = 0;
        return freeDecompressBuffers(status != success ? status : corruptedData,
                                     rle2Dec, mtfDec, bwtDec, rle1Dec);
    }

    return freeDecompressBuffers(success, rle2Dec, mtfDec, bwtDec, rle1Dec);
}

#ifndef BZIP2_TEST

#include <stdbool.h>
#include <time.h>

static int processCompressBlock(unsigned char* inputData, unsigned int dataSize,
                                FILE* outputFile,
                                unsigned long long* totalOrig,
                                unsigned long long* totalComp) {
    unsigned char* compressedData = NULL;
    unsigned int compressedSize = 0;
    errors status =
        bzip2CompressBlock(inputData, dataSize, &compressedData, &compressedSize);
    if (status != success) {
        fprintf(stderr, "Compression failed with error: %d\n", status);
        return status;
    }

    if (compressedSize > 0 && outputFile) {
        size_t written = fwrite(compressedData, 1, compressedSize, outputFile);
        if (written < compressedSize) {
            fprintf(stderr, "Error writing compressed data to file\n");
            free(compressedData);
            return fileErr;
        }
    }

    *totalOrig += dataSize;
    *totalComp += compressedSize;

    free(compressedData);
    return success;
}

static int runCompression(const char* filePath, unsigned int blockSize,
                          unsigned long long* totalOrigBytes,
                          unsigned long long* totalCompBytes) {
    char outPath[1024];
    snprintf(outPath, sizeof(outPath), "%s.bz2", filePath);

    FILE* inputFile = fopen(filePath, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open input file: %s\n", filePath);
        return fileErr;
    }

    FILE* outputFile = fopen(outPath, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to create output file: %s\n", outPath);
        fclose(inputFile);
        return fileErr;
    }

    unsigned char* inputData = malloc(blockSize);
    if (!inputData) {
        fclose(inputFile);
        fclose(outputFile);
        return mallocErr;
    }

    int blockNum = 1;
    size_t bytesRead;

    while ((bytesRead = fread(inputData, 1, blockSize, inputFile)) > 0) {
        unsigned int currentSize = (unsigned int)bytesRead;
        int status = processCompressBlock(inputData, currentSize, outputFile,
                                          totalOrigBytes, totalCompBytes);
        if (status != success) {
            free(inputData);
            fclose(inputFile);
            fclose(outputFile);
            return status;
        }
        blockNum++;
    }

    if (ferror(inputFile)) {
        fprintf(stderr, "Error reading from input file\n");
        free(inputData);
        fclose(inputFile);
        fclose(outputFile);
        return fileErr;
    }

    free(inputData);
    fclose(inputFile);
    fclose(outputFile);

    if (blockNum == 1) {
        printf("File was empty. No blocks processed.\n");
    } else {
        printf("Compressed: %llu -> %llu bytes (%.2f%%), Output: %s\n",
               *totalOrigBytes, *totalCompBytes,
               *totalOrigBytes > 0
                   ? (double)(*totalCompBytes) / (*totalOrigBytes) * 100.0
                   : 0.0,
               outPath);
    }

    return success;
}

static int runDecompression(const char* filePath,
                            unsigned long long* totalCompBytes,
                            unsigned long long* totalOrigBytes) {
    char outPath[1024];
    size_t len = strlen(filePath);
    if (len > 4 && strcmp(filePath + len - 4, ".bz2") == 0) {
        size_t baseLen = len - 4;
        snprintf(outPath, sizeof(outPath), "%.*s", (int)baseLen, filePath);
        FILE* check = fopen(outPath, "rb");
        if (check) {
            fclose(check);
            snprintf(outPath, sizeof(outPath), "%.*s.out", (int)baseLen,
                     filePath);
        }
    } else {
        snprintf(outPath, sizeof(outPath), "%s.out", filePath);
    }

    FILE* inputFile = fopen(filePath, "rb");
    if (!inputFile) {
        fprintf(stderr, "Failed to open input file: %s\n", filePath);
        return fileErr;
    }

    FILE* outputFile = fopen(outPath, "wb");
    if (!outputFile) {
        fprintf(stderr, "Failed to create output file: %s\n", outPath);
        fclose(inputFile);
        return fileErr;
    }

    int blockNum = 1;

    while (1) {
        bzip2Header header;
        size_t headerRead = fread(&header, 1, sizeof(bzip2Header), inputFile);
        if (headerRead == 0) {
            break;
        }
        if (headerRead < sizeof(bzip2Header)) {
            fprintf(stderr,
                    "Error: Truncated header in block %d (read %zu/%zu bytes)\n",
                    blockNum, headerRead, sizeof(bzip2Header));
            fclose(inputFile);
            fclose(outputFile);
            return corruptedData;
        }

        unsigned int blockSize =
            (unsigned int)sizeof(bzip2Header) + header.compressedSize;
        unsigned char* blockBuffer = malloc(blockSize);
        if (!blockBuffer) {
            fprintf(stderr,
                    "Error: Out of memory allocating %u bytes for block %d\n",
                    blockSize, blockNum);
            fclose(inputFile);
            fclose(outputFile);
            return mallocErr;
        }

        memcpy(blockBuffer, &header, sizeof(bzip2Header));

        size_t payloadRead = fread(blockBuffer + sizeof(bzip2Header), 1,
                                   header.compressedSize, inputFile);
        if (payloadRead < header.compressedSize) {
            fprintf(stderr,
                    "Error: Truncated payload in block %d (read %zu/%u bytes)\n",
                    blockNum, payloadRead, header.compressedSize);
            free(blockBuffer);
            fclose(inputFile);
            fclose(outputFile);
            return corruptedData;
        }

        unsigned char* decompressedData = NULL;
        unsigned int decompressedSize = 0;
        errors status = bzip2DecompressBlock(blockBuffer, blockSize,
                                             &decompressedData,
                                             &decompressedSize);
        free(blockBuffer);

        if (status != success) {
            fprintf(stderr, "Decompression of block %d failed with error: %d\n",
                    blockNum, status);
            fclose(inputFile);
            fclose(outputFile);
            return status;
        }

        if (decompressedSize > 0) {
            size_t written =
                fwrite(decompressedData, 1, decompressedSize, outputFile);
            if (written < decompressedSize) {
                fprintf(stderr,
                        "Error writing decompressed data for block %d\n",
                        blockNum);
                free(decompressedData);
                fclose(inputFile);
                fclose(outputFile);
                return fileErr;
            }
        }

        *totalCompBytes += blockSize;
        *totalOrigBytes += decompressedSize;

        free(decompressedData);
        blockNum++;
    }

    if (ferror(inputFile)) {
        fprintf(stderr, "Error reading from input file\n");
        fclose(inputFile);
        fclose(outputFile);
        return fileErr;
    }

    fclose(inputFile);
    fclose(outputFile);

    if (blockNum == 1) {
        printf("File was empty. No blocks decompressed.\n");
    } else {
        printf("Decompressed: %llu -> %llu bytes, Output: %s\n",
               *totalCompBytes, *totalOrigBytes, outPath);
    }

    return success;
}

int main(int argc, char** argv) {
    bool decompress = false;
    bool timed = false;
    const char* filePath = NULL;
    const char* blockSizeStr = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'd') {
                    decompress = true;
                } else if (argv[i][j] == 't') {
                    timed = true;
                } else if (argv[i][j] == 'h') {
                    fprintf(stderr,
                            "Usage: %s [-d] [-t] <file_path> [block_size_kb]\n",
                            argv[0]);
                    fprintf(stderr, "  -d : Decompress file\n");
                    fprintf(stderr,
                            "  -t : Measure and display execution time\n");
                    return success;
                } else {
                    fprintf(stderr, "Unknown option: -%c\n", argv[i][j]);
                    fprintf(stderr,
                            "Usage: %s [-d] [-t] <file_path> [block_size_kb]\n",
                            argv[0]);
                    return generalError;
                }
            }
        } else {
            if (!filePath) {
                filePath = argv[i];
            } else if (!blockSizeStr) {
                blockSizeStr = argv[i];
            } else {
                fprintf(stderr, "Unexpected extra argument: %s\n", argv[i]);
                fprintf(stderr,
                        "Usage: %s [-d] [-t] <file_path> [block_size_kb]\n",
                        argv[0]);
                return generalError;
            }
        }
    }

    if (!filePath) {
        fprintf(stderr, "Usage: %s [-d] [-t] <file_path> [block_size_kb]\n",
                argv[0]);
        fprintf(stderr, "  -d : Decompress file\n");
        fprintf(stderr, "  -t : Measure and display execution time\n");
        return generalError;
    }

    // Default block size is 900KB
    unsigned int blockSize = 900 * 1024;

    if (blockSizeStr) {
        char* endptr;
        long long kb = strtoll(blockSizeStr, &endptr, 10);

        if (endptr == blockSizeStr || *endptr != '\0') {
            fprintf(stderr, "Invalid block size. Must be a valid number.\n");
            return generalError;
        }

        if (kb <= 0) {
            fprintf(stderr, "Invalid block size. Must be > 0.\n");
            return generalError;
        }

        if (kb > (long long)(UINT_MAX / 1024)) {
            fprintf(stderr, "Block size too large. Maximum is %u KB.\n",
                    UINT_MAX / 1024);
            return sizeTooBig;
        }

        blockSize = (unsigned int)(kb * 1024);
    }

    struct timespec startTime, endTime;
    if (timed) {
        clock_gettime(CLOCK_MONOTONIC, &startTime);
    }

    unsigned long long totalOrigBytes = 0;
    unsigned long long totalCompBytes = 0;
    int status;

    if (decompress) {
        status = runDecompression(filePath, &totalCompBytes, &totalOrigBytes);
    } else {
        status = runCompression(filePath, blockSize, &totalOrigBytes,
                                &totalCompBytes);
    }

    if (status != success) {
        return status;
    }

    if (timed) {
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        double elapsed = (double)(endTime.tv_sec - startTime.tv_sec) +
                         (double)(endTime.tv_nsec - startTime.tv_nsec) / 1e9;
        printf("Total execution time: %.4f seconds\n", elapsed);
    }

    return success;
}

#endif
