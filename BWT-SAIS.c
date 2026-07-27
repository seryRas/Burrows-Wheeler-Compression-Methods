#include "BWT.h"

// true == S-type    false == L-type
// sorts types (typedOutput[i] = type of i char)
// counts amount of times there is value (charCountArr[i] = amount of ASCII[i])
// makes array of LMS indexes (LMSindexes->indexAmount - number of indexes)
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

// fills in start and end of char in array (arr[?][i] = start/end of ASCII[i])
void fillSubstringIndexes(size_t arr[2][AMOUNT_OF_VALUES], size_t* counts) {
    size_t count = 0;
    for (int i = 0; i < AMOUNT_OF_VALUES; i++) {
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
void LinductionSort(size_t* suffixArray, size_t subIndexes[2][AMOUNT_OF_VALUES], bool* typedIdx,
                    unsigned char* input, size_t inputSize) {
    size_t subBeginCpy[AMOUNT_OF_VALUES];
    memcpy(subBeginCpy, subIndexes[BEGIN], sizeof(subBeginCpy));
    size_t indexBefore;
    for (size_t i = 0; i < inputSize + 1; i++) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (typedIdx[indexBefore] == L_TYPE)
            suffixArray[(subBeginCpy[input[indexBefore]]++)] =
                indexBefore;  // input[indexBefore] == value at index before LMS
                              // index
    }
}

void SinductionSort(size_t* suffixArray, size_t subIndexes[2][AMOUNT_OF_VALUES], bool* typedIdx,
                    unsigned char* input, size_t inputSize) {
    size_t subEndCopy[AMOUNT_OF_VALUES];
    memcpy(subEndCopy, subIndexes[END], sizeof(subEndCopy));
    size_t indexBefore;
    size_t i = inputSize + 1;
    while ((i--) > 0) {
        if (suffixArray[i] == __SIZE_MAX__ || suffixArray[i] == 0) continue;
        indexBefore = suffixArray[i] - 1;

        if (typedIdx[indexBefore] == S_TYPE)
            suffixArray[(subEndCopy[input[indexBefore]]--)] = indexBefore;
    }
}

bool compareLmsSubstrings(unsigned char* input, bool* typedIdx, size_t s1Idx, size_t s2Idx, size_t inputSize) {
    bool wasLs1 = false, endS1 = false, typeS1;
    bool wasLs2 = false, endS2 = false, typeS2;
    size_t i = 0;
    while(true) {
        if(s1Idx + i == inputSize || s2Idx + i == inputSize) return false;
        if(input[s1Idx + i] != input[s2Idx + i]) return false;

        typeS1 = typedIdx[s1Idx + i];
        typeS2 = typedIdx[s2Idx + i];
        if(typeS1 != typeS2) return false;
        
        if(wasLs1 && typeS1 == S_TYPE) endS1 = true;
        if(wasLs2 && typeS2 == S_TYPE) endS2 = true;
        if(endS1 || endS2) return endS1 && endS2;

        wasLs1 = !typeS1;
        wasLs2 = !typeS2;
        i++;
    }
}

errors findSameSubstrings(unsigned char* input,size_t *sufArr, bool* typedOut, size_t len) {
    size_t index;
    size_t lastLMSIndex = __SIZE_MAX__;
    size_t name = 0;
    size_t *nameArr = malloc(sizeof(size_t) * (len + 1));
    if(!nameArr) return mallocErr;
    memset(nameArr,-1, sizeof(size_t) * (len + 1));
    size_t nameAmount = 0;
    for(size_t i = 0; i <= len; i++) {
        index = sufArr[i];
        if(index == len) {
            nameArr[index] = name++;
            nameAmount++;
            continue;
        }
        if(typedOut[index] != S_TYPE || index == 0) continue;
        if(typedOut[index - 1] != L_TYPE) continue;
        if(lastLMSIndex == __SIZE_MAX__) {
            nameArr[index] = name;
            nameAmount++;
            lastLMSIndex = index;
        }
        else {
            if(compareLmsSubstrings(input, typedOut, lastLMSIndex, index, len)) {
                nameArr[index] = name;
            }
            else {
                nameArr[index] = ++name;
                nameAmount++;
            }
            lastLMSIndex = index;
        }
    }
}

errors bwtTransform(unsigned char* input, bwt_out* output) {
    bool* typedOut = malloc(sizeof(bool) * (output->size + 1));
    if (!typedOut) return mallocErr;

    size_t charCounts[AMOUNT_OF_VALUES] = {0};

    LMSArray LMSindexes = {.indexAmount = 0};
    if (!(LMSindexes.array = malloc(sizeof(size_t) * (output->size + 1))))
        return mallocErr;

    sortTypes(output->size, input, typedOut, charCounts, &LMSindexes);

    size_t substringIndexes[2][AMOUNT_OF_VALUES];
    fillSubstringIndexes(substringIndexes, charCounts);

    size_t* suffixArr = malloc((output->size + 1) * sizeof(size_t));
    if (!suffixArr) return mallocErr;
    for (size_t i = 0; i < output->size + 1; i++)
        suffixArr[i] = __SIZE_MAX__;  // SIZE MAX signals empty space so input
                                      // must be shorter than SIZE_MAX

    size_t ssEndCopy[AMOUNT_OF_VALUES];
    memcpy(ssEndCopy, substringIndexes[END], sizeof(size_t) * AMOUNT_OF_VALUES);

    fillLMS(&LMSindexes, ssEndCopy, suffixArr, output->size, input);

    LinductionSort(suffixArr, substringIndexes, typedOut, input, output->size);
    SinductionSort(suffixArr, substringIndexes, typedOut, input, output->size);

}