#include "BWT.h"

void sortTable(unsigned char* input, size_t endInArr, size_t startInArr,
               size_t* current, size_t* swap, size_t j, size_t inputSize,
               bool simple) {
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

    if (simple) return;

    for (int i = 0; i < 0x100; i++) {
        if (count[i] > 1) {
            sortTable(input, currentValueEnd[i], valueBegin[i], current, swap,
                      j + 1, inputSize, false);
        }
    }
}

errors bwtTransform(unsigned char* input, size_t inputSize,
                    unsigned char* output) {
    if (inputSize == 0) return emptyInput;
    size_t* sortedIndexes = malloc(sizeof(size_t) * inputSize);
    if (sortedIndexes == NULL) return mallocErr;
    size_t* helperArr = malloc(sizeof(size_t) * inputSize);
    if (helperArr == NULL) {
        free(sortedIndexes);
        return mallocErr;
    }

    for (size_t i = 0; i < inputSize; i++) sortedIndexes[i] = i;
    sortTable(input, inputSize, 0, sortedIndexes, helperArr, 0, inputSize,
              false);
    free(helperArr);

    size_t initialIndex = 0;
    unsigned char* packedData = output + BWT_HEADER_SIZE;
    for (size_t i = 0; i < inputSize; i++) {
        packedData[i] = input[(sortedIndexes[i] + inputSize - 1) % inputSize];
        if (sortedIndexes[i] == 0) initialIndex = i;
    }
    memcpy(output + 1, &initialIndex, sizeof(size_t));
    free(sortedIndexes);
    return success;
}

errors bwtRetransform(unsigned char* input, size_t inputSize,
                      unsigned char* output) {
    if (inputSize == 0) return emptyInput;

    size_t payloadSize = inputSize;
    size_t initialIndex = 0;
    memcpy(&initialIndex, input + 1, sizeof(size_t));
    unsigned char* transformed = input + BWT_HEADER_SIZE;

    size_t* sortedIndexes = malloc(sizeof(size_t) * payloadSize);
    if (sortedIndexes == NULL) return mallocErr;
    size_t* helperArr = malloc(sizeof(size_t) * payloadSize);
    if (helperArr == NULL) {
        free(sortedIndexes);
        return mallocErr;
    }

    for (size_t i = 0; i < payloadSize; i++) sortedIndexes[i] = i;
    sortTable(transformed, payloadSize, 0, sortedIndexes, helperArr, 0,
              payloadSize, true);
    free(helperArr);

    size_t dataIndex = initialIndex;
    for (size_t i = 0; i < payloadSize; i++) {
        output[i] = transformed[sortedIndexes[dataIndex]];
        dataIndex = sortedIndexes[dataIndex];
    }
    return success;
}
