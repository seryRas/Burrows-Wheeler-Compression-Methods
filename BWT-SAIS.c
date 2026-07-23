#include "BWT.h"

// true == S-type    false == L-type
void sortTypes(size_t inputSize, unsigned char* input, bool* typedOutput,
               size_t* charCountArr, LMSArray* LMSindexes) {
    if (inputSize == 0) return;

    typedOutput[inputSize - 1] = L_TYPE;
    charCountArr[input[inputSize - 1]]++;
    LMSindexes->array[LMSindexes->indexAmount++] = inputSize;

    size_t i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            typedOutput[i] = typedOutput[i + 1];
        } else {
            typedOutput[i] = (input[i] < input[i + 1]) ? S_TYPE : L_TYPE;
        }

        if (typedOutput[i] == L_TYPE && typedOutput[i + 1] == S_TYPE) {
            LMSindexes->array[LMSindexes->indexAmount++] = i + 1;
        }

        charCountArr[input[i]]++;
    }
}

void fillSubstringIndexes(size_t** arr, size_t* counts) {
    size_t count = 0;
    for (int i = 0; i < 0x100; i++) {
        arr[BEGIN][i] = count;
        count += counts[i];
        arr[END][i] = count - 1;
    }
}

void fillLMS(LMSArray* indexes, size_t* substringI, size_t* suffArr,
             size_t inputSize, unsigned char* input) {
    suffArr[0] = inputSize;
    for (size_t i = 1; i < indexes->indexAmount; i++) {
        suffArr[substringI[input[indexes->array[i]]]--] =
            indexes->array[i];  // we save LMS index on ending position of char
                                // thats represented by that index also decrease
                                // the ending position of that char as it can
                                // show up again
    }
}
errors inductionSort(size_t* suffixArray) {}

errors bwtTransform(unsigned char* input, bwt_out* output) {
    bool* typedOut = malloc(sizeof(bool) * (output->size + 1));
    if (!typedOut) return mallocErr;

    size_t charCounts[0x100] = {0};

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(size_t) * (output->size + 1))))
        return mallocErr;

    sortTypes(output->size, input, typedOut, charCounts, &LMSindexes);

    size_t substringIndexes[2][0x100];
    fillSubstringIndexes(substringIndexes, charCounts);

    size_t* suffixArr = malloc((output->size + 1) * sizeof(size_t));
    if (!suffixArr) return mallocErr;
    for (size_t i = 0; i < output->size + 1; i++) suffixArr[i] = __SIZE_MAX__;

    size_t ssCopy[0x100];
    memcpy(ssCopy, substringIndexes[END], sizeof(size_t) * 0x100);

    fillLMS(&LMSindexes, substringIndexes[END], suffixArr, output->size, input);
}