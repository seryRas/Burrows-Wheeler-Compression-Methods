#include "BWT.h"

void sortTable(unsigned char* input, size_t endInArr, size_t startInArr,
               size_t* current, size_t* swap, size_t j, size_t inputSize) {
    size_t count[0x100] = {0};
    size_t valueBegin[0x100] = {0};
    size_t currentValueEnd[0x100] = {0};

    for (size_t i = startInArr; i < endInArr; i++) {
        count[input[(current[i] + j) % inputSize]]++;
    }
    size_t currentIndex = 0;
    for (int i = 0; i < 0x100; i++) {
        if (count[i] > 0) {
            if (count[i] == inputSize) return;
            valueBegin[i] = currentIndex + startInArr;
            currentValueEnd[i] = currentIndex + startInArr;
            currentIndex += count[i];
        }
    }

    for (size_t i = startInArr; i < endInArr; i++) {
        swap[currentValueEnd[input[(current[i] + j) % inputSize]]++] =
            current[i];
    }
    memcpy(current + startInArr, swap + startInArr,
           sizeof(size_t) * (endInArr - startInArr));
    for (int i = 0; i < 0x100; i++) {
        if (count[i] > 1) {
            sortTable(input, currentValueEnd[i], valueBegin[i], current, swap,
                      j + 1, inputSize);
        }
    }
}

void test(unsigned char* input, size_t* res) {
    for (size_t i = 0; i < strlen((char*)input); i++) {
        for (size_t j = 0; j < strlen((char*)input); j++) {
            size_t index = j + res[i];
            if (index >= strlen((char*)input)) index -= strlen((char*)input);
            printf("%c", input[index]);
        }
        printf("\n");
    }
}

errors bwtTransform(unsigned char* input, bwt_out* output) {
    if (output->size == 0) return emptyInput;
    size_t* sortedIndexes = malloc(sizeof(size_t) * output->size);
    if (sortedIndexes == NULL) return mallocErr;
    size_t* helperArr = malloc(sizeof(size_t) * output->size);
    if (helperArr == NULL) {
        free(sortedIndexes);
        return mallocErr;
    }

    for (size_t i = 0; i < output->size; i++) sortedIndexes[i] = i;
    sortTable(input, output->size, 0, sortedIndexes, helperArr, 0,
              output->size);
    free(helperArr);
    for (size_t i = 0; i < output->size; i++) {
        output->data[i] =
            input[(sortedIndexes[i] + output->size - 1) % output->size];
        if (sortedIndexes[i] == 0) output->initialIndex = i;
    }
    free(sortedIndexes);
    return succes;
}


errors bwtRetransform(bwt_out *input, unsigned char *output) {
    if (input->size == 0) return emptyInput;
    size_t* sortedIndexes = malloc(sizeof(size_t) * input->size);
    if (sortedIndexes == NULL) return mallocErr;
    size_t* helperArr = malloc(sizeof(size_t) * input->size);
    if (helperArr == NULL) {
        free(sortedIndexes);
        return mallocErr;
    }

    for (size_t i = 0; i < input->size; i++) sortedIndexes[i] = i;
    sortTable(input->data, input->size, 0, sortedIndexes, helperArr, 0,
              input->size);
    free(helperArr);
    
}

