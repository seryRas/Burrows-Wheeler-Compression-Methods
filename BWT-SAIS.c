#include "BWT.h"

errors sizetSAIS(unsigned int* input, rec_sais_out* output, unsigned int alphabetSize);

#define BITVECTOR_BYTE_COUNT(bitCount) (((bitCount) + 7) / 8)

static inline bool bitvectorGet(const unsigned char* bitvector, unsigned int index) {
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

static inline void fillSubstringIndexes(unsigned int* arr[2], unsigned int* counts, unsigned int alphabetSize) {
    unsigned int count = 1;
    for (unsigned int i = 0; i < alphabetSize; i++) {
        arr[BEGIN][i] = count;
        count += counts[i];
        arr[END][i] = count - 1;
    }
}

void sizetSortTypes(unsigned int inputSize, unsigned int* input, unsigned char* typedOutput,
                    unsigned int* charCountArr, LMSArray* LMSindexes) {
    if (inputSize == 0) return;

    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    LMSindexes->array[LMSindexes->indexAmount++] = inputSize;

    unsigned int i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        if (bitvectorGet(typedOutput, i) == L_TYPE &&
            bitvectorGet(typedOutput, i + 1) == S_TYPE) {
            LMSindexes->array[LMSindexes->indexAmount++] = i + 1;
        }

        charCountArr[input[i]]++;
    }
}

static inline void sizetFillLMS(LMSArray* indexes, unsigned int* substringI, unsigned int* suffArr,
                  unsigned int inputSize, unsigned int* input) {
    suffArr[0] = inputSize;
    for (unsigned int i = 1; i < indexes->indexAmount; i++) {
        suffArr[substringI[input[indexes->array[i]]]--] = indexes->array[i];
    }
}

errors sizetLinductionSort(unsigned int* suffixArray, unsigned int* subIndexes[2],
                           unsigned char* typedIdx, unsigned int* input,
                           unsigned int inputSize, unsigned int alphabetSize) {
    unsigned int* subBeginCpy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!subBeginCpy) return mallocErr;
    memcpy(subBeginCpy, subIndexes[BEGIN], sizeof(unsigned int) * alphabetSize);
    unsigned int indexBefore;
    for (unsigned int i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(subBeginCpy[input[indexBefore]]++)] = indexBefore;
    }
    free(subBeginCpy);
    return success;
}

errors sizetSinductionSort(unsigned int* suffixArray, unsigned int* subIndexes[2],
                           unsigned char* typedIdx, unsigned int* input,
                           unsigned int inputSize, unsigned int alphabetSize) {
    unsigned int* subEndCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!subEndCopy) return mallocErr;
    memcpy(subEndCopy, subIndexes[END], sizeof(unsigned int) * alphabetSize);
    unsigned int indexBefore;
    unsigned int i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(subEndCopy[input[indexBefore]]--)] = indexBefore;
    }
    free(subEndCopy);
    return success;
}

bool sizetCompareLmsSubstrings(unsigned int* input, unsigned char* typedIdx,
                               unsigned int s1Idx, unsigned int s2Idx, unsigned int inputSize) {
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

errors sizetFindSameSubstrings(unsigned int* input, unsigned int* sufArr,
                               unsigned char* typedOut, unsigned int len,
                               LMSArray* lmsArr, unsigned int** finalOrder) {
    unsigned int index;
    unsigned int lastLMSIndex = EMPTY_IDX;
    unsigned int name = 0;
    unsigned int* nameArr = malloc(sizeof(unsigned int) * (len + 1));
    if (!nameArr) return mallocErr;
    unsigned int savedNames = 0;
    unsigned int* originalLMSIndexes = malloc(sizeof(unsigned int) * (len + 1));
    if (!originalLMSIndexes) {
        free(nameArr);
        return mallocErr;
    }
    for (unsigned int i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLMSIndexes[savedNames++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLMSIndex == EMPTY_IDX) {
            originalLMSIndexes[savedNames++] = index;
            nameArr[index] = name;
            lastLMSIndex = index;
        } else {
            if (sizetCompareLmsSubstrings(input, typedOut, lastLMSIndex, index,
                                          len)) {
                nameArr[index] = name;
            } else {
                nameArr[index] = ++name;
            }
            originalLMSIndexes[savedNames++] = index;
            lastLMSIndex = index;
        }
    }

    if (name + 1 < savedNames) {
        rec_sais_out recOut = {.size = savedNames};
        unsigned int i = lmsArr->indexAmount;
        unsigned int j = 0;

        while ((i--) > 0) {
            originalLMSIndexes[j++] = nameArr[lmsArr->array[i]];
        }
        if (!(recOut.data = malloc(sizeof(unsigned int) * savedNames))) {
            free(nameArr);
            free(originalLMSIndexes);
            return mallocErr;
        }
        if (sizetSAIS(originalLMSIndexes, &recOut, name + 1) != success) {
            free(nameArr);
            free(recOut.data);
            free(originalLMSIndexes);
            return mallocErr;
        }

        for (unsigned int i = 0; i < lmsArr->indexAmount; i++) {
            unsigned int index = lmsArr->indexAmount - 1 - recOut.data[i];
            originalLMSIndexes[i] = lmsArr->array[index];
        }
        free(recOut.data);
    }

    *finalOrder = originalLMSIndexes;
    free(nameArr);
    return success;
}

static inline void sizetFinalLMSFill(unsigned int* input, unsigned int* orderedLMSindexes,
                       unsigned int indexAmount, unsigned int* ssEnd, unsigned int* sufArr) {
    sufArr[0] = orderedLMSindexes[0];
    for (unsigned int i = indexAmount - 1; i > 0; i--) {
        sufArr[ssEnd[input[orderedLMSindexes[i]]]--] = orderedLMSindexes[i];
    }
}

errors sizetSAIS(unsigned int* input, rec_sais_out* output, unsigned int alphabetSize) {
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(output->size), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    unsigned int* charCounts = calloc(alphabetSize, sizeof(unsigned int));
    if (!charCounts) {
        free(typedOut);
        return mallocErr;
    }

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(unsigned int) * (output->size + 1)))) {
        free(typedOut);
        free(charCounts);
        return mallocErr;
    }

    sizetSortTypes(output->size, input, typedOut, charCounts, &LMSindexes);

    unsigned int* substringIndexes[2];
    substringIndexes[BEGIN] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!substringIndexes[BEGIN]) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        return mallocErr;
    }
    substringIndexes[END] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!substringIndexes[END]) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        return mallocErr;
    }
    fillSubstringIndexes(substringIndexes, charCounts, alphabetSize);

    unsigned int* suffixArr = malloc((output->size + 1) * sizeof(unsigned int));
    if (!suffixArr) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        return mallocErr;
    }
    memset(suffixArr, -1, (output->size + 1) * sizeof(unsigned int));

    unsigned int* ssEndCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!ssEndCopy) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        free(suffixArr);
        return mallocErr;
    }
    memcpy(ssEndCopy, substringIndexes[END], sizeof(unsigned int) * alphabetSize);

    sizetFillLMS(&LMSindexes, ssEndCopy, suffixArr, output->size, input);

    if (sizetLinductionSort(suffixArr, substringIndexes, typedOut, input,
                            output->size, alphabetSize) != success ||
        sizetSinductionSort(suffixArr, substringIndexes, typedOut, input,
                            output->size, alphabetSize) != success) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(suffixArr);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        free(ssEndCopy);
        return mallocErr;
    }
    unsigned int* finalOrderLMSindexes;
    if (sizetFindSameSubstrings(input, suffixArr, typedOut, output->size,
                                &LMSindexes,
                                &finalOrderLMSindexes) != success) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(suffixArr);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        free(ssEndCopy);
        return mallocErr;
    }

    memset(suffixArr, -1, (output->size + 1) * sizeof(unsigned int));
    memcpy(ssEndCopy, substringIndexes[END], sizeof(unsigned int) * alphabetSize);
    sizetFinalLMSFill(input, finalOrderLMSindexes, LMSindexes.indexAmount,
                      ssEndCopy, suffixArr);

    sizetLinductionSort(suffixArr, substringIndexes, typedOut, input,
                        output->size, alphabetSize);
    sizetSinductionSort(suffixArr, substringIndexes, typedOut, input,
                        output->size, alphabetSize);

    memcpy(output->data, suffixArr + 1, sizeof(unsigned int) * output->size);
    free(typedOut);
    free(charCounts);
    free(LMSindexes.array);
    free(suffixArr);
    free(substringIndexes[BEGIN]);
    free(substringIndexes[END]);
    free(ssEndCopy);
    free(finalOrderLMSindexes);

    return success;
}

// true == S-type    false == L-type
// sorts types (typedOutput[i] = type of i char)
// counts amount of times there is value (charCountArr[i] = amount of ASCII[i])
// makes array of LMS indexes (LMSindexes->indexAmount - number of indexes)
int ucSortTypes(unsigned int inputSize, unsigned char* input,
                unsigned char* typedOutput, unsigned int* charCountArr,
                LMSArray* LMSindexes) {
    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    LMSindexes->array[LMSindexes->indexAmount++] = inputSize;

    unsigned int i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        if (bitvectorGet(typedOutput, i) == L_TYPE &&
            bitvectorGet(typedOutput, i + 1) == S_TYPE) {
            LMSindexes->array[LMSindexes->indexAmount++] = i + 1;
        }

        if ((++charCountArr[input[i]]) == inputSize) return ALL_SAME_INPUT;
    }
    return EXIT_SUCCESS;
}

// fills in start and end of char in array (arr[?][i] = start/end of ASCII[i])

static inline void ucFillLMS(LMSArray* indexes, unsigned int* substringI, unsigned int* suffArr,
               unsigned int inputSize, unsigned char* input) {
    suffArr[0] = inputSize;
    for (unsigned int i = 1; i < indexes->indexAmount; i++) {
        suffArr[substringI[input[indexes->array[i]]]--] =
            indexes->array[i];  // we save LMS index on ending position of char
                                // thats represented by that index also decrease
                                // the ending position of that char as it can
                                // show up again
    }
}
void ucLinductionSort(unsigned int* suffixArray, unsigned int* subIndexes[2],
                      unsigned char* typedIdx, unsigned char* input,
                      unsigned int inputSize) {
    unsigned int subBeginCpy[AMOUNT_OF_VALUES];
    memcpy(subBeginCpy, subIndexes[BEGIN], sizeof(subBeginCpy));
    unsigned int indexBefore;
    for (unsigned int i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(subBeginCpy[input[indexBefore]]++)] =
                indexBefore;  // input[indexBefore] == value at index before LMS
                              // index
    }
}

void ucSinductionSort(unsigned int* suffixArray, unsigned int* subIndexes[2],
                      unsigned char* typedIdx, unsigned char* input,
                      unsigned int inputSize) {
    unsigned int subEndCopy[AMOUNT_OF_VALUES];
    memcpy(subEndCopy, subIndexes[END], sizeof(subEndCopy));
    unsigned int indexBefore;
    unsigned int i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(subEndCopy[input[indexBefore]]--)] = indexBefore;
    }
}

bool ucCompareLmsSubstrings(unsigned char* input, unsigned char* typedIdx,
                            unsigned int s1Idx, unsigned int s2Idx, unsigned int inputSize) {
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

errors ucFindSameSubstrings(unsigned char* input, unsigned int* sufArr,
                            unsigned char* typedOut, unsigned int len,
                            LMSArray* lmsArr, unsigned int** finalOrder) {
    unsigned int index;
    unsigned int lastLMSIndex = EMPTY_IDX;
    unsigned int name = 0;
    unsigned int* nameArr = malloc(sizeof(unsigned int) * (len + 1));
    if (!nameArr) return mallocErr;
    unsigned int* originalLMSIndexes = malloc(sizeof(unsigned int) * (len + 1));
    if (!originalLMSIndexes) {
        free(nameArr);
        return mallocErr;
    }
    unsigned int nameAmount = 0;
    for (unsigned int i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLMSIndexes[nameAmount++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLMSIndex == EMPTY_IDX) {
            nameArr[index] = name;
            originalLMSIndexes[nameAmount++] = index;
            lastLMSIndex = index;
        } else {
            if (ucCompareLmsSubstrings(input, typedOut, lastLMSIndex, index,
                                       len)) {
                nameArr[index] = name;
            } else {
                nameArr[index] = ++name;
            }
            lastLMSIndex = index;
            originalLMSIndexes[nameAmount++] = index;
        }
    }

    if (name + 1 < nameAmount) {
        rec_sais_out out = {.size = nameAmount};
        unsigned int* denseArr = malloc(sizeof(unsigned int) * (lmsArr->indexAmount));
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
            free(originalLMSIndexes);
            return mallocErr;
        }
        if (sizetSAIS(denseArr, &out, name + 1) != success) {
            free(out.data);
            free(denseArr);
            free(nameArr);
            free(originalLMSIndexes);
            return mallocErr;
        }

        for (unsigned int i = 0; i < lmsArr->indexAmount; i++) {
            originalLMSIndexes[i] =
                lmsArr->array[lmsArr->indexAmount - 1 - out.data[i]];
        }
        free(denseArr);
        free(out.data);
    }

    *finalOrder = originalLMSIndexes;

    free(nameArr);
    return success;
}

static inline void ucFinalLMSFill(unsigned char* input, unsigned int* orderedLMSindexes,
                    unsigned int indexAmount, unsigned int* ssEnd, unsigned int* sufArr) {
    sufArr[0] = orderedLMSindexes[0];
    for (unsigned int i = indexAmount - 1; i > 0; i--) {
        sufArr[ssEnd[input[orderedLMSindexes[i]]]--] = orderedLMSindexes[i];
    }
}

errors bwtTransform(unsigned char* input, unsigned int inputSize,
                    unsigned char* output) {
    if (inputSize == 0) return emptyInput;
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(inputSize), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    unsigned int charCounts[AMOUNT_OF_VALUES] = {0};

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(unsigned int) * (inputSize + 1)))) {
        free(typedOut);
        return mallocErr;
    }

    if (ucSortTypes(inputSize, input, typedOut, charCounts, &LMSindexes) ==
        ALL_SAME_INPUT) {
        memset(output + 1, 0, sizeof(unsigned int));
        memcpy(output + BWT_HEADER_SIZE, input, inputSize);
        free(typedOut);
        free(LMSindexes.array);
        return success;
    }

    unsigned int ssIdxBegin[AMOUNT_OF_VALUES];
    unsigned int ssIdxEnd[AMOUNT_OF_VALUES];
    unsigned int* substringIndexes[2] = {ssIdxBegin, ssIdxEnd};
    fillSubstringIndexes(substringIndexes, charCounts, AMOUNT_OF_VALUES);

    unsigned int* suffixArr = malloc((inputSize + 1) * sizeof(unsigned int));
    if (!suffixArr) {
        free(typedOut);
        free(LMSindexes.array);
        return mallocErr;
    }
    memset(suffixArr, -1, (inputSize + 1) * sizeof(unsigned int));

    unsigned int ssEndCopy[AMOUNT_OF_VALUES];
    memcpy(ssEndCopy, substringIndexes[END], sizeof(unsigned int) * AMOUNT_OF_VALUES);

    ucFillLMS(&LMSindexes, ssEndCopy, suffixArr, inputSize, input);

    ucLinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    ucSinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);

    unsigned int* finalOrderLMSindexes;
    if (ucFindSameSubstrings(input, suffixArr, typedOut, inputSize, &LMSindexes,
                             &finalOrderLMSindexes) != success) {
        free(typedOut);
        free(suffixArr);
        free(LMSindexes.array);
        return mallocErr;
    }

    memset(suffixArr, -1, (inputSize + 1) * sizeof(unsigned int));
    memcpy(ssEndCopy, substringIndexes[END], sizeof(unsigned int) * AMOUNT_OF_VALUES);
    ucFinalLMSFill(input, finalOrderLMSindexes, LMSindexes.indexAmount,
                   ssEndCopy, suffixArr);

    ucLinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    ucSinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    free(finalOrderLMSindexes);

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
    free(suffixArr);
    free(LMSindexes.array);
    return success;
}
errors bwtRetransform(unsigned char* input, unsigned int inputSize,
                      unsigned char* output) {
    if (inputSize == 0) return emptyInput;

    unsigned int initialIndex = 0;
    memcpy(&initialIndex, input + 1, sizeof(unsigned int));
    unsigned char* transformed = input + BWT_HEADER_SIZE;

    if (initialIndex > inputSize) return generalError;
    unsigned int count[AMOUNT_OF_VALUES] = {0};
    for (unsigned int i = 0; i < inputSize; i++) {
        count[transformed[i]]++;
    }

    unsigned int F_start[AMOUNT_OF_VALUES] = {0};
    unsigned int sum = 1; 
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
        F_start[i] = sum;
        sum += count[i];
    }

    unsigned int* LF = malloc(sizeof(unsigned int) * inputSize);
    if (!LF) return mallocErr;

    for (unsigned int i = 0; i < inputSize; i++) {
        LF[i] = F_start[transformed[i]]++;
    }

    unsigned int curr_packed = 0;
    for (int i = (int)inputSize - 1; i >= 0; i--) {
        output[i] = transformed[curr_packed];
        unsigned int next_row = LF[curr_packed];
        
        if (next_row < initialIndex) {
            curr_packed = next_row;
        } else if (next_row > initialIndex) {
            curr_packed = next_row - 1;
        }
    }

    free(LF);
    return success;
}