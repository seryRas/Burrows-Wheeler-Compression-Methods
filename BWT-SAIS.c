#include "BWT.h"

errors recursiveSais(unsigned int* input, RecSaisOut* output,
                     unsigned int alphabetSize);

#define BITVECTOR_BYTE_COUNT(bitCount) (((bitCount) + 7) / 8)

static inline bool bitvectorGet(const unsigned char* bitvector,
                                unsigned int index) {
    return (bitvector[index / 8] >> (index % 8)) & 1u;
}

static inline void bitvectorSet(unsigned char* bitvector, unsigned int index,
                                bool value) {
    unsigned char mask = (unsigned char)(1u << (index % 8));
    if (value) {
        bitvector[index / 8] |= mask;
    } else {
        bitvector[index / 8] &= (unsigned char)~mask;
    }
}

static inline void fillBucketBounds(unsigned int* arr[2], unsigned int* counts,
                                    unsigned int alphabetSize) {
    unsigned int count = 1;
    for (unsigned int i = 0; i < alphabetSize; i++) {
        arr[BEGIN][i] = count;
        count += counts[i];
        arr[END][i] = count - 1;
    }
}

// ===================== Recursive (unsigned int) SA-IS =====================

void sortTypesRec(unsigned int inputSize, unsigned int* input,
                  unsigned char* typedOutput, unsigned int* charCountArr,
                  LmsArray* lmsIndexes) {
    if (inputSize == 0) return;

    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    lmsIndexes->array[lmsIndexes->indexAmount++] = inputSize;

    unsigned int i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        if (bitvectorGet(typedOutput, i) == L_TYPE &&
            bitvectorGet(typedOutput, i + 1) == S_TYPE) {
            lmsIndexes->array[lmsIndexes->indexAmount++] = i + 1;
        }

        charCountArr[input[i]]++;
    }
}

static inline void fillLmsRec(LmsArray* indexes, unsigned int* bucketEnd,
                              unsigned int* suffArr, unsigned int inputSize,
                              unsigned int* input) {
    suffArr[0] = inputSize;
    for (unsigned int i = 1; i < indexes->indexAmount; i++) {
        suffArr[bucketEnd[input[indexes->array[i]]]--] = indexes->array[i];
    }
}

errors lInductionSortRec(unsigned int* suffixArray,
                         unsigned int* bucketBounds[2], unsigned char* typedIdx,
                         unsigned int* input, unsigned int inputSize,
                         unsigned int alphabetSize) {
    unsigned int* bucketBeginCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketBeginCopy) return mallocErr;
    memcpy(bucketBeginCopy, bucketBounds[BEGIN],
           sizeof(unsigned int) * alphabetSize);
    unsigned int indexBefore;
    for (unsigned int i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(bucketBeginCopy[input[indexBefore]]++)] = indexBefore;
    }
    free(bucketBeginCopy);
    return success;
}

errors sInductionSortRec(unsigned int* suffixArray,
                         unsigned int* bucketBounds[2], unsigned char* typedIdx,
                         unsigned int* input, unsigned int inputSize,
                         unsigned int alphabetSize) {
    unsigned int* bucketEndCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketEndCopy) return mallocErr;
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * alphabetSize);
    unsigned int indexBefore;
    unsigned int i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(bucketEndCopy[input[indexBefore]]--)] = indexBefore;
    }
    free(bucketEndCopy);
    return success;
}

bool compareLmsSubstringsRec(unsigned int* input, unsigned char* typedIdx,
                             unsigned int s1Idx, unsigned int s2Idx,
                             unsigned int inputSize) {
    bool wasLs1 = false, endS1 = false, typeS1;
    bool wasLs2 = false, endS2 = false, typeS2;
    unsigned int i = 0;
    while (true) {
        if (s1Idx + i == inputSize || s2Idx + i == inputSize) return false;
        if (input[s1Idx + i] != input[s2Idx + i]) return false;

        typeS1 = bitvectorGet(typedIdx, s1Idx + i);
        typeS2 = bitvectorGet(typedIdx, s2Idx + i);
        if (typeS1 != typeS2) return false;

        if (wasLs1 && typeS1 == S_TYPE) endS1 = true;
        if (wasLs2 && typeS2 == S_TYPE) endS2 = true;
        if (endS1 || endS2) return endS1 && endS2;

        wasLs1 = !typeS1;
        wasLs2 = !typeS2;
        i++;
    }
}

errors findSameSubstringsRec(unsigned int* input, unsigned int* sufArr,
                             unsigned char* typedOut, unsigned int len,
                             LmsArray* lmsArr, unsigned int** finalOrder) {
    unsigned int index;
    unsigned int lastLmsIndex = EMPTY_IDX;
    unsigned int name = 0;
    unsigned int* nameArr = malloc(sizeof(unsigned int) * (len + 1));
    if (!nameArr) return mallocErr;
    unsigned int savedNames = 0;
    unsigned int* originalLmsIndexes = malloc(sizeof(unsigned int) * (len + 1));
    if (!originalLmsIndexes) {
        free(nameArr);
        return mallocErr;
    }
    for (unsigned int i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLmsIndexes[savedNames++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLmsIndex == EMPTY_IDX) {
            originalLmsIndexes[savedNames++] = index;
            nameArr[index] = name;
            lastLmsIndex = index;
        } else {
            if (compareLmsSubstringsRec(input, typedOut, lastLmsIndex, index,
                                        len)) {
                nameArr[index] = name;
            } else {
                nameArr[index] = ++name;
            }
            originalLmsIndexes[savedNames++] = index;
            lastLmsIndex = index;
        }
    }

    if (name + 1 < savedNames) {
        RecSaisOut recOut = {.size = savedNames};
        unsigned int i = lmsArr->indexAmount;
        unsigned int j = 0;

        while ((i--) > 0) {
            originalLmsIndexes[j++] = nameArr[lmsArr->array[i]];
        }
        if (!(recOut.data = malloc(sizeof(unsigned int) * savedNames))) {
            free(nameArr);
            free(originalLmsIndexes);
            return mallocErr;
        }
        if (recursiveSais(originalLmsIndexes, &recOut, name + 1) != success) {
            free(nameArr);
            free(recOut.data);
            free(originalLmsIndexes);
            return mallocErr;
        }

        for (unsigned int i = 0; i < lmsArr->indexAmount; i++) {
            unsigned int index = lmsArr->indexAmount - 1 - recOut.data[i];
            originalLmsIndexes[i] = lmsArr->array[index];
        }
        free(recOut.data);
    }

    *finalOrder = originalLmsIndexes;
    free(nameArr);
    return success;
}

static inline void finalLmsFillRec(unsigned int* input,
                                   unsigned int* orderedLmsIndexes,
                                   unsigned int indexAmount,
                                   unsigned int* bucketEnd,
                                   unsigned int* sufArr) {
    sufArr[0] = orderedLmsIndexes[0];
    for (unsigned int i = indexAmount - 1; i > 0; i--) {
        sufArr[bucketEnd[input[orderedLmsIndexes[i]]]--] = orderedLmsIndexes[i];
    }
}

errors recursiveSais(unsigned int* input, RecSaisOut* output,
                     unsigned int alphabetSize) {
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(output->size), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    unsigned int* charCounts = calloc(alphabetSize, sizeof(unsigned int));
    if (!charCounts) {
        free(typedOut);
        return mallocErr;
    }

    LmsArray lmsIndexes = {.indexAmount = 0};
    if (!(lmsIndexes.array =
              malloc(sizeof(unsigned int) * (output->size + 1)))) {
        free(typedOut);
        free(charCounts);
        return mallocErr;
    }

    sortTypesRec(output->size, input, typedOut, charCounts, &lmsIndexes);

    unsigned int* bucketBounds[2];
    bucketBounds[BEGIN] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketBounds[BEGIN]) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        return mallocErr;
    }
    bucketBounds[END] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketBounds[END]) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(bucketBounds[BEGIN]);
        return mallocErr;
    }
    fillBucketBounds(bucketBounds, charCounts, alphabetSize);

    unsigned int* suffixArr = malloc((output->size + 1) * sizeof(unsigned int));
    if (!suffixArr) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(bucketBounds[BEGIN]);
        free(bucketBounds[END]);
        return mallocErr;
    }
    memset(suffixArr, -1, (output->size + 1) * sizeof(unsigned int));

    unsigned int* bucketEndCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketEndCopy) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(bucketBounds[BEGIN]);
        free(bucketBounds[END]);
        free(suffixArr);
        return mallocErr;
    }
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * alphabetSize);

    fillLmsRec(&lmsIndexes, bucketEndCopy, suffixArr, output->size, input);

    if (lInductionSortRec(suffixArr, bucketBounds, typedOut, input,
                          output->size, alphabetSize) != success ||
        sInductionSortRec(suffixArr, bucketBounds, typedOut, input,
                          output->size, alphabetSize) != success) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(suffixArr);
        free(bucketBounds[BEGIN]);
        free(bucketBounds[END]);
        free(bucketEndCopy);
        return mallocErr;
    }
    unsigned int* finalOrderLmsIndexes;
    if (findSameSubstringsRec(input, suffixArr, typedOut, output->size,
                              &lmsIndexes, &finalOrderLmsIndexes) != success) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(suffixArr);
        free(bucketBounds[BEGIN]);
        free(bucketBounds[END]);
        free(bucketEndCopy);
        return mallocErr;
    }

    memset(suffixArr, -1, (output->size + 1) * sizeof(unsigned int));
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * alphabetSize);
    finalLmsFillRec(input, finalOrderLmsIndexes, lmsIndexes.indexAmount,
                    bucketEndCopy, suffixArr);

    if (lInductionSortRec(suffixArr, bucketBounds, typedOut, input,
                          output->size, alphabetSize) != success ||
        sInductionSortRec(suffixArr, bucketBounds, typedOut, input,
                          output->size, alphabetSize) != success) {
        free(typedOut);
        free(charCounts);
        free(lmsIndexes.array);
        free(suffixArr);
        free(bucketBounds[BEGIN]);
        free(bucketBounds[END]);
        free(bucketEndCopy);
        free(finalOrderLmsIndexes);
        return mallocErr;
    }

    memcpy(output->data, suffixArr + 1, sizeof(unsigned int) * output->size);
    free(typedOut);
    free(charCounts);
    free(lmsIndexes.array);
    free(suffixArr);
    free(bucketBounds[BEGIN]);
    free(bucketBounds[END]);
    free(bucketEndCopy);
    free(finalOrderLmsIndexes);

    return success;
}

// true == S-type    false == L-type
// sorts types (typedOutput[i] = type of i char)
// counts amount of times there is value (charCountArr[i] = amount of ASCII[i])
// makes array of LMS indexes (lmsIndexes->indexAmount - number of indexes)
int sortTypes(unsigned int inputSize, unsigned char* input,
              unsigned char* typedOutput, unsigned int* charCountArr,
              LmsArray* lmsIndexes) {
    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    lmsIndexes->array[lmsIndexes->indexAmount++] = inputSize;

    unsigned int i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        if (bitvectorGet(typedOutput, i) == L_TYPE &&
            bitvectorGet(typedOutput, i + 1) == S_TYPE) {
            lmsIndexes->array[lmsIndexes->indexAmount++] = i + 1;
        }

        if ((++charCountArr[input[i]]) == inputSize) return ALL_SAME_INPUT;
    }
    return EXIT_SUCCESS;
}

static inline void fillLms(LmsArray* indexes, unsigned int* bucketEnd,
                           unsigned int* suffArr, unsigned int inputSize,
                           unsigned char* input) {
    suffArr[0] = inputSize;
    for (unsigned int i = 1; i < indexes->indexAmount; i++) {
        suffArr[bucketEnd[input[indexes->array[i]]]--] = indexes->array[i];
    }
}

void lInductionSort(unsigned int* suffixArray, unsigned int* bucketBounds[2],
                    unsigned char* typedIdx, unsigned char* input,
                    unsigned int inputSize) {
    unsigned int bucketBeginCopy[AMOUNT_OF_VALUES];
    memcpy(bucketBeginCopy, bucketBounds[BEGIN], sizeof(bucketBeginCopy));
    unsigned int indexBefore;
    for (unsigned int i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(bucketBeginCopy[input[indexBefore]]++)] = indexBefore;
    }
}

void sInductionSort(unsigned int* suffixArray, unsigned int* bucketBounds[2],
                    unsigned char* typedIdx, unsigned char* input,
                    unsigned int inputSize) {
    unsigned int bucketEndCopy[AMOUNT_OF_VALUES];
    memcpy(bucketEndCopy, bucketBounds[END], sizeof(bucketEndCopy));
    unsigned int indexBefore;
    unsigned int i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(bucketEndCopy[input[indexBefore]]--)] = indexBefore;
    }
}

bool compareLmsSubstrings(unsigned char* input, unsigned char* typedIdx,
                          unsigned int s1Idx, unsigned int s2Idx,
                          unsigned int inputSize) {
    bool wasLs1 = false, endS1 = false, typeS1;
    bool wasLs2 = false, endS2 = false, typeS2;
    unsigned int i = 0;
    while (true) {
        if (s1Idx + i == inputSize || s2Idx + i == inputSize) return false;
        if (input[s1Idx + i] != input[s2Idx + i]) return false;

        typeS1 = bitvectorGet(typedIdx, s1Idx + i);
        typeS2 = bitvectorGet(typedIdx, s2Idx + i);
        if (typeS1 != typeS2) return false;

        if (wasLs1 && typeS1 == S_TYPE) endS1 = true;
        if (wasLs2 && typeS2 == S_TYPE) endS2 = true;
        if (endS1 || endS2) return endS1 && endS2;

        wasLs1 = !typeS1;
        wasLs2 = !typeS2;
        i++;
    }
}

errors findSameSubstrings(unsigned char* input, unsigned int* sufArr,
                          unsigned char* typedOut, unsigned int len,
                          LmsArray* lmsArr, unsigned int** finalOrder) {
    unsigned int index;
    unsigned int lastLmsIndex = EMPTY_IDX;
    unsigned int name = 0;
    unsigned int* nameArr = malloc(sizeof(unsigned int) * (len + 1));
    if (!nameArr) return mallocErr;
    unsigned int* originalLmsIndexes = malloc(sizeof(unsigned int) * (len + 1));
    if (!originalLmsIndexes) {
        free(nameArr);
        return mallocErr;
    }
    unsigned int nameAmount = 0;
    for (unsigned int i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLmsIndexes[nameAmount++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLmsIndex == EMPTY_IDX) {
            nameArr[index] = name;
            originalLmsIndexes[nameAmount++] = index;
            lastLmsIndex = index;
        } else {
            if (compareLmsSubstrings(input, typedOut, lastLmsIndex, index,
                                     len)) {
                nameArr[index] = name;
            } else {
                nameArr[index] = ++name;
            }
            lastLmsIndex = index;
            originalLmsIndexes[nameAmount++] = index;
        }
    }

    if (name + 1 < nameAmount) {
        RecSaisOut out = {.size = nameAmount};
        unsigned int* denseArr =
            malloc(sizeof(unsigned int) * (lmsArr->indexAmount));
        if (!denseArr) {
            free(nameArr);
            return mallocErr;
        }
        unsigned int j = lmsArr->indexAmount;
        unsigned int i = 0;
        while ((j--) > 0) {
            denseArr[i++] = nameArr[lmsArr->array[j]];
        }

        if (!(out.data = malloc(sizeof(unsigned int) * nameAmount))) {
            free(denseArr);
            free(nameArr);
            free(originalLmsIndexes);
            return mallocErr;
        }
        if (recursiveSais(denseArr, &out, name + 1) != success) {
            free(out.data);
            free(denseArr);
            free(nameArr);
            free(originalLmsIndexes);
            return mallocErr;
        }

        for (unsigned int i = 0; i < lmsArr->indexAmount; i++) {
            originalLmsIndexes[i] =
                lmsArr->array[lmsArr->indexAmount - 1 - out.data[i]];
        }
        free(denseArr);
        free(out.data);
    }

    *finalOrder = originalLmsIndexes;

    free(nameArr);
    return success;
}

static inline void finalLmsFill(unsigned char* input,
                                unsigned int* orderedLmsIndexes,
                                unsigned int indexAmount,
                                unsigned int* bucketEnd, unsigned int* sufArr) {
    sufArr[0] = orderedLmsIndexes[0];
    for (unsigned int i = indexAmount - 1; i > 0; i--) {
        sufArr[bucketEnd[input[orderedLmsIndexes[i]]]--] = orderedLmsIndexes[i];
    }
}

errors bwtTransform(unsigned char* input, unsigned int inputSize,
                    unsigned char* output) {
    if (inputSize == 0) return emptyInput;
    if (inputSize >= UINT_MAX) return sizeTooBig;

    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(inputSize), sizeof(unsigned char));
    unsigned int* lmsArray = malloc(sizeof(unsigned int) * (inputSize + 1));
    unsigned int* suffixArr = malloc((inputSize + 1) * sizeof(unsigned int));

    if (!typedOut || !lmsArray || !suffixArr) {
        free(typedOut);
        free(lmsArray);
        free(suffixArr);
        return mallocErr;
    }

    memset(typedOut, 0, BITVECTOR_BYTE_COUNT(inputSize));
    memset(suffixArr, -1, (inputSize + 1) * sizeof(unsigned int));

    LmsArray lmsIndexes = {.indexAmount = 0, .array = lmsArray};
    unsigned int charCounts[AMOUNT_OF_VALUES] = {0};

    if (sortTypes(inputSize, input, typedOut, charCounts, &lmsIndexes) ==
        ALL_SAME_INPUT) {
        unsigned int zeroIndex = 0;
        memcpy(output + 1, &zeroIndex, sizeof(unsigned int));
        memcpy(output + BWT_HEADER_SIZE, input, inputSize);
        free(typedOut);
        free(lmsArray);
        free(suffixArr);
        return success;
    }

    unsigned int bucketBegin[AMOUNT_OF_VALUES];
    unsigned int bucketEnd[AMOUNT_OF_VALUES];
    unsigned int* bucketBounds[2] = {bucketBegin, bucketEnd};
    fillBucketBounds(bucketBounds, charCounts, AMOUNT_OF_VALUES);

    unsigned int bucketEndCopy[AMOUNT_OF_VALUES];
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * AMOUNT_OF_VALUES);

    fillLms(&lmsIndexes, bucketEndCopy, suffixArr, inputSize, input);

    lInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);
    sInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);

    unsigned int* finalOrderLmsIndexes;
    if (findSameSubstrings(input, suffixArr, typedOut, inputSize, &lmsIndexes,
                           &finalOrderLmsIndexes) != success) {
        free(typedOut);
        free(lmsArray);
        free(suffixArr);
        return mallocErr;
    }

    memset(suffixArr, -1, (inputSize + 1) * sizeof(unsigned int));
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * AMOUNT_OF_VALUES);
    finalLmsFill(input, finalOrderLmsIndexes, lmsIndexes.indexAmount,
                 bucketEndCopy, suffixArr);

    lInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);
    sInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);
    free(finalOrderLmsIndexes);

    unsigned int initialIndex = 0;
    unsigned char* packedData = output + BWT_HEADER_SIZE;
    unsigned int packedIdx = 0;

    for (unsigned int i = 0; i <= inputSize; i++) {
        if (suffixArr[i] == 0) {
            initialIndex = i;
        } else {
            packedData[packedIdx++] = input[suffixArr[i] - 1];
        }
    }
    memcpy(output + 1, &initialIndex, sizeof(unsigned int));

    free(typedOut);
    free(lmsArray);
    free(suffixArr);
    return success;
}

errors bwtRetransform(unsigned char* input, unsigned int inputSize,
                      unsigned char* output) {
    if (inputSize == 0) return emptyInput;
    if (inputSize >= UINT_MAX) return sizeTooBig;

    unsigned int initialIndex = 0;
    memcpy(&initialIndex, input + 1, sizeof(unsigned int));
    unsigned char* transformed = input + BWT_HEADER_SIZE;

    if (initialIndex > inputSize) {
        return generalError;
    }

    unsigned int* lf = malloc(sizeof(unsigned int) * inputSize);
    if (!lf) return mallocErr;

    unsigned int count[AMOUNT_OF_VALUES] = {0};
    for (unsigned int i = 0; i < inputSize; i++) {
        count[transformed[i]]++;
    }

    unsigned int fStart[AMOUNT_OF_VALUES] = {0};
    unsigned int sum = 1;
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
        fStart[i] = sum;
        sum += count[i];
    }

    for (unsigned int i = 0; i < inputSize; i++) {
        lf[i] = fStart[transformed[i]]++;
    }

    unsigned int currPacked = 0;
    for (int i = (int)inputSize - 1; i >= 0; i--) {
        output[i] = transformed[currPacked];
        unsigned int nextRow = lf[currPacked];

        if (nextRow < initialIndex) {
            currPacked = nextRow;
        } else if (nextRow > initialIndex) {
            currPacked = nextRow - 1;
        } else {
            currPacked = 0;
        }
    }

    free(lf);
    return success;
}
