#include <time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BWT.h"

void print_peak_memory(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Peak RAM usage: %ld KB (%.2f MB)\n", 
           usage.ru_maxrss, usage.ru_maxrss / 1024.0);
}
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return generalError;
    }

    FILE* testInput = fopen(argv[1], "rb");
    if (!testInput) {
        fprintf(stderr, "Failed to open file\n");
        return fileErr;
    }

    fseek(testInput, 0, SEEK_END);
    long toldSize = ftell(testInput);
    if (toldSize <= 0) {
        fclose(testInput);
        return emptyInput;
    }
    unsigned int fileSize = (unsigned int)toldSize;
    rewind(testInput);

    unsigned char* arr = malloc(fileSize + 1);
    if (!arr) {
        fclose(testInput);
        return mallocErr;
    }

    size_t bytesRead = fread(arr, 1, fileSize, testInput);
    fclose(testInput);
    arr[bytesRead] = '\0';

    while (bytesRead > 0 && (arr[bytesRead - 1] == '\n' || arr[bytesRead - 1] == '\r')) {
        arr[--bytesRead] = '\0';
    }
    fileSize = (unsigned int)bytesRead;

    if (fileSize == 0) {
        free(arr);
        return emptyInput;
    }

    // Calculate how many chunks we need to process
    unsigned int numChunks = (fileSize + MAX_CHUNK - 1) / MAX_CHUNK;
    
    // Each chunk requires its own BWT header size
    unsigned char* packed = malloc(fileSize + (numChunks * BWT_HEADER_SIZE) + 1);
    if (!packed) {
        free(arr);
        return mallocErr;
    }

    unsigned char* restored = malloc(fileSize + 1);
    if (!restored) {
        free(packed);
        free(arr);
        return mallocErr;
    }
    restored[fileSize] = '\0';

    double totalTransformMs = 0.0;
    double totalRetransformMs = 0.0;

    for (unsigned int offset = 0, chunkIdx = 0; offset < fileSize; offset += MAX_CHUNK, chunkIdx++) {
        unsigned int currentChunkSize = fileSize - offset;
        if (currentChunkSize > MAX_CHUNK) {
            currentChunkSize = MAX_CHUNK;
        }

        unsigned char* currentArr = arr + offset;
        unsigned char* currentPacked = packed + offset + (chunkIdx * BWT_HEADER_SIZE);
        unsigned char* currentRestored = restored + offset;

        clock_t startTime = clock();
        if (bwtTransform(currentArr, currentChunkSize, currentPacked) != success) {
            fprintf(stderr, "Transform failed on chunk %u\n", chunkIdx);
            free(restored);
            free(packed);
            free(arr);
            return generalError;
        }
        clock_t endTime = clock();
        totalTransformMs += ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;

        startTime = clock();
        if (bwtRetransform(currentPacked, currentChunkSize, currentRestored) != success) {
            fprintf(stderr, "Retransform failed on chunk %u\n", chunkIdx);
            free(restored);
            free(packed);
            free(arr);
            return generalError;
        }
        endTime = clock();
        totalRetransformMs += ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;
    }

    fprintf(stdout, "Total Transform time: %f ms\n", totalTransformMs);
    fprintf(stdout, "Total Retransform time: %f ms\n", totalRetransformMs);

    // Using memcmp is safer for binary/chunked data than strcmp
    if (memcmp(restored, arr, fileSize) != 0) {
        fprintf(stderr, "FAIL: Restored data does not match original.\n");
    } else {
        fprintf(stdout, "SUCCESS: Data verified.\n");
    }

    free(restored);
    free(packed);
    free(arr);
    print_peak_memory();
    
    return success;
}