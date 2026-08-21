#include "BWT.h"

errors sizetSAIS(size_t* input, rec_sais_out* output, size_t alphabetSize);

#define BITVECTOR_BYTE_COUNT(bitCount) (((bitCount) + 7) / 8)

static inline bool bitvectorGet(const unsigned char* bitvector, size_t index) {
    return (bitvector[index / 8] >> (index % 8)) & 1u;
}

static inline void bitvectorSet(unsigned char* bitvector, size_t index,
                                bool value) {
    unsigned char mask = (unsigned char)(1u << (index % 8));
    if (value) {
        bitvector[index / 8] |= mask;
    } else {
        bitvector[index / 8] &= (unsigned char)~mask;
    }
}

static inline void fillSubstringIndexes(size_t* arr[2], size_t* counts, size_t alphabetSize) {
    size_t count = 1;
    for (size_t i = 0; i < alphabetSize; i++) {
        arr[BEGIN][i] = count;
        count += counts[i];
        arr[END][i] = count - 1;
    }
}

void sizetSortTypes(size_t inputSize, size_t* input, unsigned char* typedOutput,
                    size_t* charCountArr, LMSArray* LMSindexes) {
    if (inputSize == 0) return;

    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    LMSindexes->array[LMSindexes->indexAmount++] = inputSize;

    size_t i = inputSize - 1;
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

static inline void sizetFillLMS(LMSArray* indexes, size_t* substringI, size_t* suffArr,
                  size_t inputSize, size_t* input) {
    suffArr[0] = inputSize;
    for (size_t i = 1; i < indexes->indexAmount; i++) {
        suffArr[substringI[input[indexes->array[i]]]--] = indexes->array[i];
    }
}

errors sizetLinductionSort(size_t* suffixArray, size_t* subIndexes[2],
                           unsigned char* typedIdx, size_t* input,
                           size_t inputSize, size_t alphabetSize) {
    size_t* subBeginCpy = malloc(sizeof(size_t) * alphabetSize);
    if (!subBeginCpy) return mallocErr;
    memcpy(subBeginCpy, subIndexes[BEGIN], sizeof(size_t) * alphabetSize);
    size_t indexBefore;
    for (size_t i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(subBeginCpy[input[indexBefore]]++)] = indexBefore;
    }
    free(subBeginCpy);
    return success;
}

errors sizetSinductionSort(size_t* suffixArray, size_t* subIndexes[2],
                           unsigned char* typedIdx, size_t* input,
                           size_t inputSize, size_t alphabetSize) {
    size_t* subEndCopy = malloc(sizeof(size_t) * alphabetSize);
    if (!subEndCopy) return mallocErr;
    memcpy(subEndCopy, subIndexes[END], sizeof(size_t) * alphabetSize);
    size_t indexBefore;
    size_t i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(subEndCopy[input[indexBefore]]--)] = indexBefore;
    }
    free(subEndCopy);
    return success;
}

bool sizetCompareLmsSubstrings(size_t* input, unsigned char* typedIdx,
                               size_t s1Idx, size_t s2Idx, size_t inputSize) {
    bool wasLs1 = false, endS1 = false, typeS1;
    bool wasLs2 = false, endS2 = false, typeS2;
    size_t i = 0;
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

errors sizetFindSameSubstrings(size_t* input, size_t* sufArr,
                               unsigned char* typedOut, size_t len,
                               LMSArray* lmsArr, size_t** finalOrder) {
    size_t index;
    size_t lastLMSIndex = __SIZE_MAX__;
    size_t name = 0;
    size_t* nameArr = malloc(sizeof(size_t) * (len + 1));
    if (!nameArr) return mallocErr;
    size_t savedNames = 0;
    size_t* originalLMSIndexes = malloc(sizeof(size_t) * (len + 1));
    if (!originalLMSIndexes) {
        free(nameArr);
        return mallocErr;
    }
    for (size_t i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLMSIndexes[savedNames++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLMSIndex == __SIZE_MAX__) {
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
        size_t i = lmsArr->indexAmount;
        size_t j = 0;

        while ((i--) > 0) {
            originalLMSIndexes[j++] = nameArr[lmsArr->array[i]];
        }
        if (!(recOut.data = malloc(sizeof(size_t) * savedNames))) {
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

        for (size_t i = 0; i < lmsArr->indexAmount; i++) {
            size_t index = lmsArr->indexAmount - 1 - recOut.data[i];
            originalLMSIndexes[i] = lmsArr->array[index];
        }
        free(recOut.data);
    }

    *finalOrder = originalLMSIndexes;
    free(nameArr);
    return success;
}

static inline void sizetFinalLMSFill(size_t* input, size_t* orderedLMSindexes,
                       size_t indexAmount, size_t* ssEnd, size_t* sufArr) {
    sufArr[0] = orderedLMSindexes[0];
    for (size_t i = indexAmount - 1; i > 0; i--) {
        sufArr[ssEnd[input[orderedLMSindexes[i]]]--] = orderedLMSindexes[i];
    }
}

errors sizetSAIS(size_t* input, rec_sais_out* output, size_t alphabetSize) {
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(output->size), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    size_t* charCounts = calloc(alphabetSize, sizeof(size_t));
    if (!charCounts) {
        free(typedOut);
        return mallocErr;
    }

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(size_t) * (output->size + 1)))) {
        free(typedOut);
        free(charCounts);
        return mallocErr;
    }

    sizetSortTypes(output->size, input, typedOut, charCounts, &LMSindexes);

    size_t* substringIndexes[2];
    substringIndexes[BEGIN] = malloc(sizeof(size_t) * alphabetSize);
    if (!substringIndexes[BEGIN]) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        return mallocErr;
    }
    substringIndexes[END] = malloc(sizeof(size_t) * alphabetSize);
    if (!substringIndexes[END]) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        return mallocErr;
    }
    fillSubstringIndexes(substringIndexes, charCounts, alphabetSize);

    size_t* suffixArr = malloc((output->size + 1) * sizeof(size_t));
    if (!suffixArr) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        return mallocErr;
    }
    memset(suffixArr, -1, (output->size + 1) * sizeof(size_t));

    size_t* ssEndCopy = malloc(sizeof(size_t) * alphabetSize);
    if (!ssEndCopy) {
        free(typedOut);
        free(charCounts);
        free(LMSindexes.array);
        free(substringIndexes[BEGIN]);
        free(substringIndexes[END]);
        free(suffixArr);
        return mallocErr;
    }
    memcpy(ssEndCopy, substringIndexes[END], sizeof(size_t) * alphabetSize);

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
    size_t* finalOrderLMSindexes;
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

    memset(suffixArr, -1, (output->size + 1) * sizeof(size_t));
    memcpy(ssEndCopy, substringIndexes[END], sizeof(size_t) * alphabetSize);
    sizetFinalLMSFill(input, finalOrderLMSindexes, LMSindexes.indexAmount,
                      ssEndCopy, suffixArr);

    sizetLinductionSort(suffixArr, substringIndexes, typedOut, input,
                        output->size, alphabetSize);
    sizetSinductionSort(suffixArr, substringIndexes, typedOut, input,
                        output->size, alphabetSize);

    memcpy(output->data, suffixArr + 1, sizeof(size_t) * output->size);
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
int ucSortTypes(size_t inputSize, unsigned char* input,
                unsigned char* typedOutput, size_t* charCountArr,
                LMSArray* LMSindexes) {
    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;
    LMSindexes->array[LMSindexes->indexAmount++] = inputSize;

    size_t i = inputSize - 1;
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

static inline void ucFillLMS(LMSArray* indexes, size_t* substringI, size_t* suffArr,
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
void ucLinductionSort(size_t* suffixArray, size_t* subIndexes[2],
                      unsigned char* typedIdx, unsigned char* input,
                      size_t inputSize) {
    size_t subBeginCpy[AMOUNT_OF_VALUES];
    memcpy(subBeginCpy, subIndexes[BEGIN], sizeof(subBeginCpy));
    size_t indexBefore;
    for (size_t i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(subBeginCpy[input[indexBefore]]++)] =
                indexBefore;  // input[indexBefore] == value at index before LMS
                              // index
    }
}

void ucSinductionSort(size_t* suffixArray, size_t* subIndexes[2],
                      unsigned char* typedIdx, unsigned char* input,
                      size_t inputSize) {
    size_t subEndCopy[AMOUNT_OF_VALUES];
    memcpy(subEndCopy, subIndexes[END], sizeof(subEndCopy));
    size_t indexBefore;
    size_t i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(subEndCopy[input[indexBefore]]--)] = indexBefore;
    }
}

bool ucCompareLmsSubstrings(unsigned char* input, unsigned char* typedIdx,
                            size_t s1Idx, size_t s2Idx, size_t inputSize) {
    bool wasLs1 = false, endS1 = false, typeS1;
    bool wasLs2 = false, endS2 = false, typeS2;
    size_t i = 0;
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

errors ucFindSameSubstrings(unsigned char* input, size_t* sufArr,
                            unsigned char* typedOut, size_t len,
                            LMSArray* lmsArr, size_t** finalOrder) {
    size_t index;
    size_t lastLMSIndex = __SIZE_MAX__;
    size_t name = 0;
    size_t* nameArr = malloc(sizeof(size_t) * (len + 1));
    if (!nameArr) return mallocErr;
    size_t* originalLMSIndexes = malloc(sizeof(size_t) * (len + 1));
    if (!originalLMSIndexes) {
        free(nameArr);
        return mallocErr;
    }
    size_t nameAmount = 0;
    for (size_t i = 0; i <= len; i++) {
        index = sufArr[i];
        if (index == len) {
            originalLMSIndexes[nameAmount++] = index;
            nameArr[index] = name++;
            continue;
        }
        if (bitvectorGet(typedOut, index) != S_TYPE || index == 0) continue;
        if (bitvectorGet(typedOut, index - 1) != L_TYPE) continue;
        if (lastLMSIndex == __SIZE_MAX__) {
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
        size_t* denseArr = malloc(sizeof(size_t) * (lmsArr->indexAmount));
        if (!denseArr) {
            free(nameArr);
            return mallocErr;  // add frees for other arrays
        }
        size_t j = lmsArr->indexAmount;
        size_t i = 0;
        while ((j--) > 0) {
            denseArr[i++] = nameArr[lmsArr->array[j]];
        }

        if (!(out.data = malloc(sizeof(size_t) * nameAmount))) {
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

        for (size_t i = 0; i < lmsArr->indexAmount; i++) {
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

static inline void ucFinalLMSFill(unsigned char* input, size_t* orderedLMSindexes,
                    size_t indexAmount, size_t* ssEnd, size_t* sufArr) {
    sufArr[0] = orderedLMSindexes[0];
    for (size_t i = indexAmount - 1; i > 0; i--) {
        sufArr[ssEnd[input[orderedLMSindexes[i]]]--] = orderedLMSindexes[i];
    }
}

errors bwtTransform(unsigned char* input, size_t inputSize,
                    unsigned char* output) {
    if (inputSize == 0) return emptyInput;
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(inputSize), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    size_t charCounts[AMOUNT_OF_VALUES] = {0};

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(size_t) * (inputSize + 1)))) {
        free(typedOut);
        return mallocErr;
    }

    if (ucSortTypes(inputSize, input, typedOut, charCounts, &LMSindexes) ==
        ALL_SAME_INPUT) {
        memset(output + 1, 0, sizeof(size_t));
        memcpy(output + BWT_HEADER_SIZE, input, inputSize);
        free(typedOut);
        free(LMSindexes.array);
        return success;
    }

    size_t ssIdxBegin[AMOUNT_OF_VALUES];
    size_t ssIdxEnd[AMOUNT_OF_VALUES];
    size_t* substringIndexes[2] = {ssIdxBegin, ssIdxEnd};
    fillSubstringIndexes(substringIndexes, charCounts, AMOUNT_OF_VALUES);

    size_t* suffixArr = malloc((inputSize + 1) * sizeof(size_t));
    if (!suffixArr) {
        free(typedOut);
        free(LMSindexes.array);
        return mallocErr;
    }
    memset(suffixArr, -1, (inputSize + 1) * sizeof(size_t));
    // SIZE MAX signals empty space so input
    // must be shorter than SIZE_MAX

    size_t ssEndCopy[AMOUNT_OF_VALUES];
    memcpy(ssEndCopy, substringIndexes[END], sizeof(size_t) * AMOUNT_OF_VALUES);

    ucFillLMS(&LMSindexes, ssEndCopy, suffixArr, inputSize, input);

    ucLinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    ucSinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);

    size_t* finalOrderLMSindexes;
    if (ucFindSameSubstrings(input, suffixArr, typedOut, inputSize, &LMSindexes,
                             &finalOrderLMSindexes) != success) {
        free(typedOut);
        free(suffixArr);
        free(LMSindexes.array);
        return mallocErr;
    }

    memset(suffixArr, -1, (inputSize + 1) * sizeof(size_t));
    memcpy(ssEndCopy, substringIndexes[END], sizeof(size_t) * AMOUNT_OF_VALUES);
    ucFinalLMSFill(input, finalOrderLMSindexes, LMSindexes.indexAmount,
                   ssEndCopy, suffixArr);

    ucLinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    ucSinductionSort(suffixArr, substringIndexes, typedOut, input, inputSize);
    free(finalOrderLMSindexes);

    size_t initialIndex = 0;
    unsigned char* packedData = output + BWT_HEADER_SIZE;
    for (size_t i = 1; i <= inputSize; i++) {
        if (suffixArr[i] == 0) {
            packedData[i - 1] = input[inputSize - 1];
            initialIndex = i - 1;
        } else {
            packedData[i - 1] = input[suffixArr[i] - 1];
        }
    }
    memcpy(output + 1, &initialIndex, sizeof(size_t));
    free(typedOut);
    free(suffixArr);
    free(LMSindexes.array);
    return success;
}

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
    free(sortedIndexes);
    return success;
}