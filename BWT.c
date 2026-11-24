#include "BWT.h"

#include <assert.h>

static inline void freeAll(void* p1, void* p2, void* p3, void* p4, void* p5,
                           void* p6, void* p7, void* p8) {
    free(p1);
    free(p2);
    free(p3);
    free(p4);
    free(p5);
    free(p6);
    free(p7);
    free(p8);
}

errors recursiveSais(unsigned int* input, unsigned int* suffixArr,
                     unsigned int inputSize, unsigned int alphabetSize);

#define BITVECTOR_BYTE_COUNT(bitCount) (((size_t)(bitCount) + 7) / 8)

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

// index is LMS when it is S-type after L-type so index 0 cant be LMS
// sentinel at the end is S-type after L-type so its always LMS
// no two LMS can be next to each other so there is at most half of them
static inline bool isLms(const unsigned char* typedOut, unsigned int index) {
    return index > 0 && bitvectorGet(typedOut, index) == S_TYPE &&
           bitvectorGet(typedOut, index - 1) == L_TYPE;
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

// starts from 0 because reduced string already has 0 as sentinel at the end
static inline void fillBucketBoundsRec(unsigned int* arr[2],
                                       unsigned int* counts,
                                       unsigned int alphabetSize) {
    unsigned int count = 0;
    for (unsigned int i = 0; i < alphabetSize; i++) {
        arr[BEGIN][i] = count;
        count += counts[i];
        arr[END][i] = count - 1;
    }
}

// ===================== Recursive (unsigned int) SA-IS =====================

// last element is sentinel 0 so its S-type and the one before is L-type
void sortTypesRec(unsigned int inputSize, unsigned int* input,
                  unsigned char* typedOutput, unsigned int* charCountArr) {
    if (inputSize == 0) return;

    bitvectorSet(typedOutput, inputSize - 1, S_TYPE);
    charCountArr[input[inputSize - 1]]++;
    if (inputSize < 2) return;

    bitvectorSet(typedOutput, inputSize - 2, L_TYPE);
    charCountArr[input[inputSize - 2]]++;

    unsigned int i = inputSize - 2;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        charCountArr[input[i]]++;
    }
}

static inline void fillLmsRec(unsigned char* typedOut, unsigned int* bucketEnd,
                              unsigned int* suffArr, unsigned int inputSize,
                              unsigned int* input) {
    for (unsigned int i = 1; i < inputSize; i++) {
        if (isLms(typedOut, i)) {
            suffArr[bucketEnd[input[i]]--] = i;
        }
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
    for (unsigned int i = 0; i < inputSize; i++) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;
        assert(indexBefore != EMPTY_IDX);

        if (bitvectorGet(typedIdx, indexBefore) == L_TYPE)
            suffixArray[(bucketBeginCopy[input[indexBefore]]++)] = indexBefore;
    }
    freeAll(bucketBeginCopy, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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
    unsigned int i = inputSize;
    while ((i--) > 0) {
        if (suffixArray[i] == EMPTY_IDX || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (bitvectorGet(typedIdx, indexBefore) == S_TYPE)
            suffixArray[(bucketEndCopy[input[indexBefore]]--)] = indexBefore;
    }
    freeAll(bucketEndCopy, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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

// packs LMS to the start of sufArr and writes names at lmsCount + index/2
// index/2 cant collide because LMS arent adjacent and stays inside sufArr
// if duplicates exist packs reduced string to the right end and runs recursiveSais in place
errors findSameSubstringsRec(unsigned int* input, unsigned int* sufArr,
                             unsigned char* typedOut, unsigned int len,
                             unsigned int* lmsCountOut) {
    unsigned int lmsCount = 0;
    for (unsigned int i = 0; i < len; i++) {
        if (isLms(typedOut, sufArr[i])) {
            sufArr[lmsCount++] = sufArr[i];
        }
    }

    for (unsigned int i = lmsCount; i < len; i++) sufArr[i] = EMPTY_IDX;

    unsigned int lastLmsIndex = EMPTY_IDX;
    unsigned int name = 0;
    for (unsigned int i = 0; i < lmsCount; i++) {
        unsigned int index = sufArr[i];
        if (lastLmsIndex == EMPTY_IDX) {
            lastLmsIndex = index;
        } else {
            if (!compareLmsSubstringsRec(input, typedOut, lastLmsIndex, index,
                                         len - 1)) {
                name++;
            }
            lastLmsIndex = index;
        }
        sufArr[lmsCount + (index / 2)] = name;
    }

    if (name + 1 < lmsCount) {
        unsigned int dest = len - 1;
        for (unsigned int i = len; i-- > lmsCount;) {
            if (sufArr[i] != EMPTY_IDX) {
                sufArr[dest--] = sufArr[i];
            }
        }
        unsigned int* reducedStr = sufArr + len - lmsCount;

        errors _err2 = recursiveSais(reducedStr, sufArr, lmsCount, name + 1);
        if (_err2 != success) return _err2;

        unsigned int j = 0;
        for (unsigned int i = 1; i < len; i++) {
            if (isLms(typedOut, i)) {
                reducedStr[j++] = i;
            }
        }
        for (unsigned int k = 0; k < lmsCount; k++) {
            sufArr[k] = reducedStr[sufArr[k]];
        }
    }

    *lmsCountOut = lmsCount;
    return success;
}

// moves sorted LMS from start of sufArr to bucket ends from right to left
// works in place because bucketEnd is always >= i so it never overwrites unread values
static inline void finalLmsFillRec(unsigned int* input,
                                   unsigned int indexAmount,
                                   unsigned int* bucketEnd,
                                   unsigned int* sufArr) {
    for (unsigned int i = indexAmount; i-- > 0;) {
        unsigned int index = sufArr[i];
        sufArr[i] = EMPTY_IDX;
        sufArr[bucketEnd[input[index]]--] = index;
    }
}

errors recursiveSais(unsigned int* input, unsigned int* suffixArr,
                     unsigned int inputSize, unsigned int alphabetSize) {
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(inputSize), sizeof(unsigned char));
    if (!typedOut) return mallocErr;

    unsigned int* charCounts = calloc(alphabetSize, sizeof(unsigned int));
    if (!charCounts) {
        freeAll(typedOut, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        return mallocErr;
    }

    sortTypesRec(inputSize, input, typedOut, charCounts);

    unsigned int* bucketBounds[2];
    bucketBounds[BEGIN] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketBounds[BEGIN]) {
        freeAll(typedOut, charCounts, NULL, NULL, NULL, NULL, NULL, NULL);
        return mallocErr;
    }
    bucketBounds[END] = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketBounds[END]) {
        freeAll(typedOut, charCounts, bucketBounds[BEGIN], NULL, NULL, NULL,
                NULL, NULL);
        return mallocErr;
    }
    fillBucketBoundsRec(bucketBounds, charCounts, alphabetSize);

    for (unsigned int _idx = 0; _idx < inputSize; _idx++)
        suffixArr[_idx] = EMPTY_IDX;

    unsigned int* bucketEndCopy = malloc(sizeof(unsigned int) * alphabetSize);
    if (!bucketEndCopy) {
        freeAll(typedOut, charCounts, bucketBounds[BEGIN], bucketBounds[END],
                NULL, NULL, NULL, NULL);
        return mallocErr;
    }
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * alphabetSize);

    fillLmsRec(typedOut, bucketEndCopy, suffixArr, inputSize, input);

    if (lInductionSortRec(suffixArr, bucketBounds, typedOut, input, inputSize,
                          alphabetSize) != success ||
        sInductionSortRec(suffixArr, bucketBounds, typedOut, input, inputSize,
                          alphabetSize) != success) {
        freeAll(typedOut, charCounts, bucketBounds[BEGIN], bucketBounds[END],
                bucketEndCopy, NULL, NULL, NULL);
        return mallocErr;
    }

    unsigned int lmsCount = 0;
    errors _err1 = findSameSubstringsRec(input, suffixArr, typedOut, inputSize,
                                         &lmsCount);
    if (_err1 != success) {
        freeAll(typedOut, charCounts, bucketBounds[BEGIN], bucketBounds[END],
                bucketEndCopy, NULL, NULL, NULL);
        return _err1;
    }

    for (unsigned int _idx = lmsCount; _idx < inputSize; _idx++)
        suffixArr[_idx] = EMPTY_IDX;
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * alphabetSize);
    finalLmsFillRec(input, lmsCount, bucketEndCopy, suffixArr);

    if (lInductionSortRec(suffixArr, bucketBounds, typedOut, input, inputSize,
                          alphabetSize) != success ||
        sInductionSortRec(suffixArr, bucketBounds, typedOut, input, inputSize,
                          alphabetSize) != success) {
        freeAll(typedOut, charCounts, bucketBounds[BEGIN], bucketBounds[END],
                bucketEndCopy, NULL, NULL, NULL);
        return mallocErr;
    }

    freeAll(typedOut, charCounts, bucketBounds[BEGIN], bucketBounds[END],
            bucketEndCopy, NULL, NULL, NULL);

    return success;
}

// true == S-type    false == L-type
// sorts types (typedOutput[i] = type of i char, sentinel at inputSize)
// counts amount of times there is value (charCountArr[i] = amount of ASCII[i])
int sortTypes(unsigned int inputSize, unsigned char* input,
              unsigned char* typedOutput, unsigned int* charCountArr) {
    bitvectorSet(typedOutput, inputSize, S_TYPE);
    bitvectorSet(typedOutput, inputSize - 1, L_TYPE);
    charCountArr[input[inputSize - 1]]++;

    unsigned int i = inputSize - 1;
    while (i-- > 0) {
        if (input[i] == input[i + 1]) {
            bitvectorSet(typedOutput, i, bitvectorGet(typedOutput, i + 1));
        } else {
            bitvectorSet(typedOutput, i, input[i] < input[i + 1]);
        }

        if ((++charCountArr[input[i]]) == inputSize) return ALL_SAME_INPUT;
    }
    return EXIT_SUCCESS;
}

static inline void fillLms(unsigned char* typedOut, unsigned int* bucketEnd,
                           unsigned int* suffArr, unsigned int inputSize,
                           unsigned char* input) {
    suffArr[0] = inputSize;
    for (unsigned int i = 1; i < inputSize; i++) {
        if (isLms(typedOut, i)) {
            suffArr[bucketEnd[input[i]]--] = i;
        }
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
        assert(indexBefore != EMPTY_IDX);

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

// packs LMS to the start of sufArr and writes names at lmsCount + index/2
// index/2 cant collide because LMS arent adjacent and stays inside sufArr
// if duplicates exist packs reduced string to the right end and runs recursiveSais in place
errors findSameSubstrings(unsigned char* input, unsigned int* sufArr,
                          unsigned char* typedOut, unsigned int len,
                          unsigned int* lmsCountOut) {
    unsigned int saLen = len + 1;

    unsigned int lmsCount = 0;
    for (unsigned int i = 0; i < saLen; i++) {
        if (isLms(typedOut, sufArr[i])) {
            sufArr[lmsCount++] = sufArr[i];
        }
    }

    for (unsigned int i = lmsCount; i < saLen; i++) sufArr[i] = EMPTY_IDX;

    unsigned int lastLmsIndex = EMPTY_IDX;
    unsigned int name = 0;
    for (unsigned int i = 0; i < lmsCount; i++) {
        unsigned int index = sufArr[i];
        if (lastLmsIndex == EMPTY_IDX) {
            lastLmsIndex = index;
        } else {
            if (!compareLmsSubstrings(input, typedOut, lastLmsIndex, index,
                                      len)) {
                name++;
            }
            lastLmsIndex = index;
        }
        sufArr[lmsCount + (index / 2)] = name;
    }

    if (name + 1 < lmsCount) {
        unsigned int dest = saLen - 1;
        for (unsigned int i = saLen; i-- > lmsCount;) {
            if (sufArr[i] != EMPTY_IDX) {
                sufArr[dest--] = sufArr[i];
            }
        }
        unsigned int* reducedStr = sufArr + saLen - lmsCount;

        errors _err2 = recursiveSais(reducedStr, sufArr, lmsCount, name + 1);
        if (_err2 != success) return _err2;

        unsigned int j = 0;
        for (unsigned int i = 1; i < saLen; i++) {
            if (isLms(typedOut, i)) {
                reducedStr[j++] = i;
            }
        }
        for (unsigned int k = 0; k < lmsCount; k++) {
            sufArr[k] = reducedStr[sufArr[k]];
        }
    }

    *lmsCountOut = lmsCount;
    return success;
}

// moves sorted LMS from start of sufArr to bucket ends from right to left
// sufArr[0] is sentinel so stops at 1 and bucketEnd is always >= i
static inline void finalLmsFill(unsigned char* input, unsigned int indexAmount,
                                unsigned int* bucketEnd, unsigned int* sufArr) {
    for (unsigned int i = indexAmount; i-- > 1;) {
        unsigned int index = sufArr[i];
        sufArr[i] = EMPTY_IDX;
        sufArr[bucketEnd[input[index]]--] = index;
    }
}

errors bwtTransform(unsigned char* input, unsigned int inputSize,
                    unsigned char* output, unsigned int* bwtIndex) {
    if (inputSize == 0) return emptyInput;
    if (inputSize >= UINT_MAX) return sizeTooBig;

    unsigned int saLen = inputSize + 1;
    unsigned char* typedOut =
        calloc(BITVECTOR_BYTE_COUNT(saLen), sizeof(unsigned char));
    unsigned int* suffixArr = malloc((size_t)saLen * sizeof(unsigned int));

    if (!typedOut || !suffixArr) {
        freeAll(typedOut, suffixArr, NULL, NULL, NULL, NULL, NULL, NULL);
        return mallocErr;
    }

    memset(typedOut, 0, BITVECTOR_BYTE_COUNT(saLen));
    for (unsigned int _idx = 0; _idx < saLen; _idx++)
        suffixArr[_idx] = EMPTY_IDX;

    unsigned int charCounts[AMOUNT_OF_VALUES] = {0};

    if (sortTypes(inputSize, input, typedOut, charCounts) == ALL_SAME_INPUT) {
        *bwtIndex = inputSize;
        memcpy(output, input, inputSize);
        freeAll(typedOut, suffixArr, NULL, NULL, NULL, NULL, NULL, NULL);
        return success;
    }

    unsigned int bucketBegin[AMOUNT_OF_VALUES];
    unsigned int bucketEnd[AMOUNT_OF_VALUES];
    unsigned int* bucketBounds[2] = {bucketBegin, bucketEnd};
    fillBucketBounds(bucketBounds, charCounts, AMOUNT_OF_VALUES);

    unsigned int bucketEndCopy[AMOUNT_OF_VALUES];
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * AMOUNT_OF_VALUES);

    fillLms(typedOut, bucketEndCopy, suffixArr, inputSize, input);

    lInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);
    sInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);

    unsigned int lmsCount = 0;
    errors _err3 =
        findSameSubstrings(input, suffixArr, typedOut, inputSize, &lmsCount);
    if (_err3 != success) {
        freeAll(typedOut, suffixArr, NULL, NULL, NULL, NULL, NULL, NULL);
        return _err3;
    }

    for (unsigned int _idx = lmsCount; _idx < saLen; _idx++)
        suffixArr[_idx] = EMPTY_IDX;
    memcpy(bucketEndCopy, bucketBounds[END],
           sizeof(unsigned int) * AMOUNT_OF_VALUES);
    finalLmsFill(input, lmsCount, bucketEndCopy, suffixArr);

    lInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);
    sInductionSort(suffixArr, bucketBounds, typedOut, input, inputSize);

    unsigned int packedIdx = 0;
    for (unsigned int i = 0; i <= inputSize; i++) {
        if (suffixArr[i] == 0) {
            *bwtIndex = i;
        } else {
            output[packedIdx++] = input[suffixArr[i] - 1];
        }
    }

    freeAll(typedOut, suffixArr, NULL, NULL, NULL, NULL, NULL, NULL);
    return success;
}

errors bwtRetransform(unsigned char* input, unsigned int inputSize,
                      unsigned char* output, unsigned int bwtIndex) {
    if (inputSize == 0) return emptyInput;
    if (inputSize >= UINT_MAX) return sizeTooBig;

    if (bwtIndex > inputSize || bwtIndex == 0) {
        return generalError;
    }

    unsigned int* lf = malloc(sizeof(unsigned int) * inputSize);
    if (!lf) return mallocErr;

    unsigned int count[AMOUNT_OF_VALUES] = {0};
    for (unsigned int i = 0; i < inputSize; i++) {
        count[input[i]]++;
    }

    unsigned int fStart[AMOUNT_OF_VALUES] = {0};
    unsigned int sum = 1;
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
        fStart[i] = sum;
        sum += count[i];
    }

    for (unsigned int i = 0; i < inputSize; i++) {
        lf[i] = fStart[input[i]]++;
    }

    unsigned int currPacked = 0;
    for (unsigned int i = inputSize; i-- > 0;) {
        output[i] = input[currPacked];
        unsigned int nextRow = lf[currPacked];

        if (nextRow < bwtIndex) {
            currPacked = nextRow;
        } else if (nextRow > bwtIndex) {
            currPacked = nextRow - 1;
        } else {
            currPacked = 0;
        }
    }

    freeAll(lf, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    return success;
}
