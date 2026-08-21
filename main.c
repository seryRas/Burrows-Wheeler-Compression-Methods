#include <time.h>
#include <sys/resource.h>

#include "BWT.h"

void print_peak_memory(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    // ru_maxrss is in Kilobytes on Linux
    printf("Peak RAM usage: %ld KB (%.2f MB)\n", 
           usage.ru_maxrss, usage.ru_maxrss / 1024.0);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "You need to run this program with file path\n");
        return generalError;
    }
    FILE* testInput = fopen(argv[1], "r");
    if (!testInput) {
        fprintf(stderr, "Failed to open file\n");
        return fileErr;
    }

    fseek(testInput, 0, SEEK_END);
    __uint32_t fileSize = ftell(testInput);
    rewind(testInput);

    unsigned char* arr = malloc(fileSize + 1);
    if (!arr) {
        fprintf(stderr, "malloc failed\n");
        fclose(testInput);
        return mallocErr;
    }

    fread(arr, 1, fileSize, testInput);
    arr[fileSize] = '\0';

    unsigned char* packed = malloc(fileSize + BWT_HEADER_SIZE);
    if (!packed) {
        free(arr);
        fclose(testInput);
        fprintf(stderr, "malloc failed\n");
        return mallocErr;
    }

    unsigned char* restored = malloc(fileSize + 1);
    if (!restored) {
        free(packed);
        fclose(testInput);
        fprintf(stderr, "malloc failed\n");
        return mallocErr;
    }
    restored[fileSize] = '\0';

    clock_t startTime = clock();
    if (bwtTransform((unsigned char*)arr, fileSize, packed) != success) {
        free(restored);
        free(packed);
        free(arr);
        fclose(testInput);
        return generalError;
    }
    clock_t endTime = clock();
    double transformMs =
        ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;

    startTime = clock();
    if (bwtRetransform(packed, fileSize, restored) != success) {
        free(restored);
        free(packed);
        free(arr);
        fclose(testInput);
        return generalError;
    }
    endTime = clock();
    double retransformMs =
        ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;

    fprintf(stdout, "Transform time: %f ms\n", transformMs);
    fprintf(stdout, "Retransform time: %f ms\n", retransformMs);

    free(restored);
    free(packed);
    free(arr);
    fclose(testInput);
    print_peak_memory();
    return success;
}
