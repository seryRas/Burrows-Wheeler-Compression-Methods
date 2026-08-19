#include <time.h>

#include "BWT.h"

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
    size_t fileSize = ftell(testInput);
    rewind(testInput);

    unsigned char* arr = malloc(fileSize + 1);
    if (!arr) {
        fprintf(stderr, "malloc failed\n");
        fclose(testInput);
        return mallocErr;
    }

    fread(arr, 1, fileSize, testInput);
    arr[fileSize] = '\0';

    bwt_out out = {.size = fileSize};
    if ((out.data = malloc(fileSize + 1)) == NULL) return mallocErr;
    out.data[fileSize] = '\0';

    unsigned char* restored = malloc(fileSize + 1);
    if (!restored) {
        free(out.data);
        fclose(testInput);
        fprintf(stderr, "malloc failed\n");
        return mallocErr;
    }
    restored[fileSize] = '\0';

    clock_t startTime = clock();
    if (bwtTransform((unsigned char*)arr, &out) != success) {
        free(restored);
        free(out.data);
        free(arr);
        fclose(testInput);
        return generalError;
    }
    clock_t endTime = clock();
    double transformMs =
        ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;

    startTime = clock();
    if (bwtRetransform(&out, restored) != success) {
        free(restored);
        free(out.data);
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
    free(out.data);
    free(arr);
    fclose(testInput);
    return success;
}
