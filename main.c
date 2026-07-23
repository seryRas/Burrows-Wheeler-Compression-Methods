#include "BWT.h"
#include <time.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "You need to run this program with file path\n");
        return generalError;
    }
    FILE *testInput = fopen(argv[1], "r");
    if(!testInput) {
        fprintf(stderr, "Failed to open file\n");
        return fileErr;
    }

    fseek(testInput, 0, SEEK_END);
    size_t fileSize = ftell(testInput);
    rewind(testInput);

    unsigned char *arr = malloc(fileSize + 1);
    if(!arr) {
        fprintf(stderr, "malloc failed\n");
        fclose(testInput);
        return mallocErr;
    }
    
    fread(arr, 1, fileSize, testInput);
    arr[fileSize] = '\0';


    bwt_out out = {.size = fileSize};
    if ((out.data = malloc(fileSize)) == NULL) return mallocErr;

    clock_t startTime = clock();
    bwtTransform((unsigned char*)arr, &out);
    clock_t endTime = clock();
    double elapsedMs = ((double)(endTime - startTime) / CLOCKS_PER_SEC) * 1000;
    fprintf(stdout, "Execution time: %f ms\n", elapsedMs);
    return succes;
}
