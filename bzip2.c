#include "bzip2.h"
#include <time.h>

static inline double getElapsedTime(struct timespec* start,
                                    struct timespec* end) {
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

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

errors bzip2CompressBlockTimed(unsigned char* inputData, unsigned int dataSize,
                               unsigned char** outputData,
                               unsigned int* outputSize,
                               bzip2StepTiming* timing) {
    if (dataSize == 0) {
        *outputData = NULL;
        *outputSize = 0;
        return success;
    }

    struct timespec stepStart, stepEnd;

    bzip2Header header;
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
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    status =
        RLE1encode(inputData, dataSize, &rle1Out, &rle1Size, header.inUseMap);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->rle1Time += getElapsedTime(&stepStart, &stepEnd);
    }
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);
    header.rle1Size = rle1Size;

    // 2. BWT
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    bwtOut = malloc(rle1Size);
    if (!bwtOut)
        return freeCompressBuffers(mallocErr, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    status = bwtTransform(rle1Out, rle1Size, bwtOut, &header.bwtIndex);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->bwtTime += getElapsedTime(&stepStart, &stepEnd);
    }
    free(rle1Out);
    rle1Out = NULL;
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    // 3. MTF
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
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
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->mtfTime += getElapsedTime(&stepStart, &stepEnd);
    }
    free(bwtOut);
    bwtOut = NULL;
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);

    // 4. RLE 2
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    unsigned int frequencyMap[ALPHABET_SIZE] = {0};
    status = RLE2encode(mtfOut, rle1Size, &rle2Out, &rle2Size, frequencyMap);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->rle2Time += getElapsedTime(&stepStart, &stepEnd);
    }
    free(mtfOut);
    mtfOut = NULL;
    if (status != success)
        return freeCompressBuffers(status, rle1Out, bwtOut, mtfOut, rle2Out,
                                   huffOut);
    header.rle2Size = rle2Size;

    // 5. Huffman
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    status =
        huffmanEncode(frequencyMap, rle2Out, &huffOut, rle2Size, &huffSize);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->huffmanTime += getElapsedTime(&stepStart, &stepEnd);
    }
    free(rle2Out);
    rle2Out = NULL;
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

errors bzip2CompressBlock(unsigned char* inputData, unsigned int dataSize,
                          unsigned char** outputData,
                          unsigned int* outputSize) {
    return bzip2CompressBlockTimed(inputData, dataSize, outputData, outputSize,
                                   NULL);
}

errors bzip2DecompressBlockTimed(unsigned char* inputData,
                                 unsigned int inputSize,
                                 unsigned char** outputData,
                                 unsigned int* outputSize,
                                 bzip2StepTiming* timing) {
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

    struct timespec stepStart, stepEnd;
    errors status;
    unsigned char* huffIn = inputData + sizeof(bzip2Header);
    unsigned char* rle2Dec = NULL;
    unsigned char* mtfDec = NULL;
    unsigned char* bwtDec = NULL;
    unsigned char* rle1Dec = NULL;
    unsigned int rle2DecSize = 0;
    unsigned int mtfDecSize = 0;

    // 1. Huffman Decode
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    status = huffmanDecode(huffIn, header.compressedSize, &rle2Dec,
                           &rle2DecSize, header.rle2Size);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->huffmanTime += getElapsedTime(&stepStart, &stepEnd);
    }
    if (status != success || rle2DecSize != header.rle2Size) {
        return freeDecompressBuffers(status != success ? status : corruptedData,
                                     rle2Dec, mtfDec, bwtDec, rle1Dec);
    }

    // 2. RLE 2 Decode
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    status =
        RLE2decode(rle2Dec, rle2DecSize, &mtfDec, &mtfDecSize, header.rle1Size);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->rle2Time += getElapsedTime(&stepStart, &stepEnd);
    }
    free(rle2Dec);
    rle2Dec = NULL;
    if (status != success || mtfDecSize != header.rle1Size) {
        return freeDecompressBuffers(status != success ? status : corruptedData,
                                     rle2Dec, mtfDec, bwtDec, rle1Dec);
    }

    // 3. MTF Decode
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
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
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->mtfTime += getElapsedTime(&stepStart, &stepEnd);
    }
    free(mtfDec);
    mtfDec = NULL;
    if (status != success)
        return freeDecompressBuffers(status, rle2Dec, mtfDec, bwtDec, rle1Dec);

    // 4. BWT Retransform
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    rle1Dec = malloc(header.rle1Size);
    if (!rle1Dec)
        return freeDecompressBuffers(mallocErr, rle2Dec, mtfDec, bwtDec,
                                     rle1Dec);

    status = bwtRetransform(bwtDec, header.rle1Size, rle1Dec, header.bwtIndex);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->bwtTime += getElapsedTime(&stepStart, &stepEnd);
    }
    free(bwtDec);
    bwtDec = NULL;
    if (status != success)
        return freeDecompressBuffers(status, rle2Dec, mtfDec, bwtDec, rle1Dec);

    // 5. RLE 1 Decode
    if (timing) clock_gettime(CLOCK_MONOTONIC, &stepStart);
    RLE1info rle1Info;
    RLE1infoInit(&rle1Info, rle1Dec, header.rle1Size);
    status = RLE1decode(&rle1Info, outputData, outputSize, header.origSize);
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &stepEnd);
        timing->rle1Time += getElapsedTime(&stepStart, &stepEnd);
    }
    free(rle1Dec);
    rle1Dec = NULL;
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

errors bzip2DecompressBlock(unsigned char* inputData, unsigned int inputSize,
                            unsigned char** outputData,
                            unsigned int* outputSize) {
    return bzip2DecompressBlockTimed(inputData, inputSize, outputData,
                                     outputSize, NULL);
}

#ifndef BZIP2_TEST

#include <stdbool.h>
#include <time.h>

static int processCompressBlock(unsigned char* inputData, unsigned int dataSize,
                                FILE* outputFile,
                                unsigned long long* totalOrig,
                                unsigned long long* totalComp,
                                bzip2StepTiming* stepTiming) {
    unsigned char* compressedData = NULL;
    unsigned int compressedSize = 0;
    errors status =
        bzip2CompressBlockTimed(inputData, dataSize, &compressedData,
                                &compressedSize, stepTiming);
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
                          unsigned long long* totalCompBytes,
                          bzip2StepTiming* stepTiming) {
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
                                          totalOrigBytes, totalCompBytes,
                                          stepTiming);
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
                            unsigned long long* totalOrigBytes,
                            bzip2StepTiming* stepTiming) {
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
        errors status = bzip2DecompressBlockTimed(blockBuffer, blockSize,
                                                  &decompressedData,
                                                  &decompressedSize,
                                                  stepTiming);
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
    bzip2StepTiming stepTiming = {0};
    int status;

    if (decompress) {
        status = runDecompression(filePath, &totalCompBytes, &totalOrigBytes,
                                  timed ? &stepTiming : NULL);
    } else {
        status = runCompression(filePath, blockSize, &totalOrigBytes,
                                &totalCompBytes,
                                timed ? &stepTiming : NULL);
    }

    if (status != success) {
        return status;
    }

    if (timed) {
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        double elapsed = (double)(endTime.tv_sec - startTime.tv_sec) +
                         (double)(endTime.tv_nsec - startTime.tv_nsec) / 1e9;
        double sumSteps = stepTiming.rle1Time + stepTiming.bwtTime +
                          stepTiming.mtfTime + stepTiming.rle2Time +
                          stepTiming.huffmanTime;

        printf("\n--- Step Timing Breakdown ---\n");
        printf("Total execution time: %.4f s\n", elapsed);
        if (decompress) {
            printf("Decompression steps:\n");
            printf("  1. Huffman Decode : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.huffmanTime, stepTiming.huffmanTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.huffmanTime / sumSteps) * 100.0 : 0.0);
            printf("  2. RLE 2 Decode   : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.rle2Time, stepTiming.rle2Time * 1000.0,
                   sumSteps > 0 ? (stepTiming.rle2Time / sumSteps) * 100.0 : 0.0);
            printf("  3. MTF Decode     : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.mtfTime, stepTiming.mtfTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.mtfTime / sumSteps) * 100.0 : 0.0);
            printf("  4. BWT Retransform: %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.bwtTime, stepTiming.bwtTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.bwtTime / sumSteps) * 100.0 : 0.0);
            printf("  5. RLE 1 Decode   : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.rle1Time, stepTiming.rle1Time * 1000.0,
                   sumSteps > 0 ? (stepTiming.rle1Time / sumSteps) * 100.0 : 0.0);
        } else {
            printf("Compression steps:\n");
            printf("  1. RLE 1 Encode   : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.rle1Time, stepTiming.rle1Time * 1000.0,
                   sumSteps > 0 ? (stepTiming.rle1Time / sumSteps) * 100.0 : 0.0);
            printf("  2. BWT Transform  : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.bwtTime, stepTiming.bwtTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.bwtTime / sumSteps) * 100.0 : 0.0);
            printf("  3. MTF Encode     : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.mtfTime, stepTiming.mtfTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.mtfTime / sumSteps) * 100.0 : 0.0);
            printf("  4. RLE 2 Encode   : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.rle2Time, stepTiming.rle2Time * 1000.0,
                   sumSteps > 0 ? (stepTiming.rle2Time / sumSteps) * 100.0 : 0.0);
            printf("  5. Huffman Encode : %8.4f s (%8.2f ms) [%5.1f%%]\n",
                   stepTiming.huffmanTime, stepTiming.huffmanTime * 1000.0,
                   sumSteps > 0 ? (stepTiming.huffmanTime / sumSteps) * 100.0 : 0.0);
        }
        double otherTime = elapsed - sumSteps;
        if (otherTime < 0) otherTime = 0;
        printf("  I/O & overhead    : %8.4f s (%8.2f ms) [%5.1f%%]\n",
               otherTime, otherTime * 1000.0,
               elapsed > 0 ? (otherTime / elapsed) * 100.0 : 0.0);
        printf("-----------------------------\n");
    }

    return success;
}

#endif
