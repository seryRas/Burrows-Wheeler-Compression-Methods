#include "BWT.h"

void sortTable(unsigned char* input, size_t inputSize, size_t* result, size_t j,
               size_t* resultIndex) {
    size_t index;
    sortingArray arr[0xFF] = {0};
    for (size_t i = 0; i < inputSize; i++) {
        index = i + j;
        if (index >= inputSize) index -= inputSize;
        arr[input[index]].indexArray[arr[input[index]].currentLen++] = i;
    }

    for (int i = 0; i < 0xFF; i++) {
        if (arr[i].currentLen > 1) {
            sortTable(input, inputSize, result, j + 1, resultIndex);
        } else if (arr[i].currentLen == 1)
            result[(*resultIndex)++] = arr[i].indexArray[0];
    }

}

void test(unsigned char *input, size_t *res) {
    for(int i = 0; i < strlen(input); i++) {
        for(int j = 0; j < strlen(input); j++) {
            int index = j + res[i];
            if(index >= strlen(input)) index -= strlen(input);
            printf("%c", input[index]);
        }
        printf("\n");
    }
}

unsigned short bwtTransform(unsigned char* input, bwt_out* output) {
    size_t* sortedIndexes = malloc(sizeof(size_t) * output->size);
    size_t finalIndex = 0;
    sortTable(input, output->size, sortedIndexes, 0, &finalIndex);
    for(size_t i = 0; i < finalIndex; i++) printf("%lu\n", sortedIndexes[i]);
    test(input, sortedIndexes);
    free(sortedIndexes);
    return 0;
}