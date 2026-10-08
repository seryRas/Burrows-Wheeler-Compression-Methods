#include "rle.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

errors RLE1encode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize,
                  unsigned char* inUseMap) {
    if (inputSize == 0) {
        *outputSize = 0;
        *output = NULL;
        return success;
    }

    if (inputSize > UINT_MAX - (inputSize / SEQ_MAXLEN) - 1) return sizeTooBig;

    *output = malloc(inputSize + (inputSize / SEQ_MAXLEN) + 1);
    if (!(*output)) return mallocErr;

    unsigned int count = 1;
    unsigned int index = 0;
    unsigned int outputIndex = 0;
    unsigned char val;

    val = input[index];
    inUseMap[val / 8] |= (1 << (val % 8));
    (*output)[outputIndex++] = val;

    unsigned char lastChar = input[index++];

    while (index != inputSize) {
        if (input[index] == lastChar) {
            count++;
            if (count == UCHAR_MAX + SEQ_MAXLEN) {
                val = count - SEQ_MAXLEN;
                inUseMap[val / 8] |= (1 << (val % 8));
                (*output)[outputIndex++] = val;
                count = 0;
                index++;
                continue;
            }
            if (count > SEQ_MAXLEN) {
                index++;
                continue;
            }
        } else {
            if (count >= SEQ_MAXLEN) {
                val = count - SEQ_MAXLEN;
                inUseMap[val / 8] |= (1 << (val % 8));
                (*output)[outputIndex++] = val;
            }
            lastChar = input[index];
            count = 1;
        }

        val = input[index++];
        inUseMap[val / 8] |= (1 << (val % 8));
        (*output)[outputIndex++] = val;
    }

    if (count >= SEQ_MAXLEN) {
        val = count - SEQ_MAXLEN;
        inUseMap[val / 8] |= (1 << (val % 8));
        (*output)[outputIndex++] = val;
    }

    (*outputSize) = outputIndex;
    return success;
}

void RLE1infoInit(RLE1info* data, unsigned char* input,
                  unsigned int inputSize) {
    data->next = input;
    data->remainingSize = inputSize;
    data->remainingToAdd = 0;
    data->lastChar = 0;
    data->charCount = 0;
    data->done = (inputSize == 0);
}

errors RLE1decode(RLE1info* data, unsigned char** output,
                  unsigned int* outputSize, unsigned int chunkSize) {
    *output = malloc(chunkSize);
    if (!(*output)) return mallocErr;

    unsigned int index = 0;
    unsigned int outputIndex = 0;
    unsigned char lastChar = data->lastChar;
    unsigned char count = data->charCount;
    unsigned int remaining = data->remainingSize;
    unsigned char* input = data->next;

    while (outputIndex < chunkSize) {
        if (data->remainingToAdd > 0) {
            (*output)[outputIndex++] = lastChar;
            data->remainingToAdd--;
            continue;
        }

        if (index >= remaining) {
            if (count == SEQ_MAXLEN) {
                free(*output);
                *output = NULL;
                *outputSize = 0;
                return corruptedData;
            }
            break;
        }

        if (count == SEQ_MAXLEN) {
            data->remainingToAdd = input[index++];
            count = 0;
            continue;
        }

        unsigned char c = input[index++];
        (*output)[outputIndex++] = c;

        if (c == lastChar) {
            count++;
        } else {
            lastChar = c;
            count = 1;
        }
    }

    data->lastChar = lastChar;
    data->charCount = count;
    data->remainingSize = remaining - index;
    data->next = input + index;
    data->done = (data->remainingSize == 0 && data->remainingToAdd == 0);

    *outputSize = outputIndex;
    return success;
}

errors RLE2encode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize) {
    if (inputSize > (UINT_MAX - 2) / 2) return sizeTooBig;

    *output = malloc(inputSize * 2 + 2);
    if (!(*output)) return mallocErr;

    unsigned int inIdx = 0;
    unsigned int outIdx = 0;

    while (inIdx < inputSize) {
        if (input[inIdx] == 0) {
            unsigned int zeroCount = 0;
            while (inIdx < inputSize && input[inIdx] == 0) {
                zeroCount++;
                inIdx++;
            }

            while (zeroCount > 0) {
                unsigned char chunk = (zeroCount > UCHAR_MAX)
                                          ? UCHAR_MAX
                                          : (unsigned char)zeroCount;
                (*output)[outIdx++] = 0;
                (*output)[outIdx++] = chunk;
                zeroCount -= chunk;
            }
        } else {
            (*output)[outIdx++] = input[inIdx++];
        }
    }

    (*output)[outIdx++] = 0;
    (*output)[outIdx++] = 0;

    *outputSize = outIdx;
    return success;
}

static errors corruptedRLE2(unsigned char** output, unsigned int* outputSize) {
    free(*output);
    *output = NULL;
    *outputSize = 0;
    return corruptedData;
}

errors RLE2decode(unsigned char* input, unsigned int inputSize,
                  unsigned char** output, unsigned int* outputSize,
                  unsigned int expectedOutputSize) {
    if (expectedOutputSize == 0) {
        *output = NULL;
        *outputSize = 0;
        return success;
    }

    *output = malloc(expectedOutputSize);
    if (!(*output)) return mallocErr;

    unsigned int inIdx = 0;
    unsigned int outIdx = 0;
    bool eobFound = false;

    while (inIdx < inputSize) {
        unsigned char c = input[inIdx++];

        if (c != 0) {
            if (outIdx >= expectedOutputSize)
                return corruptedRLE2(output, outputSize);
            (*output)[outIdx++] = c;
            continue;
        }

        if (inIdx >= inputSize) return corruptedRLE2(output, outputSize);
        unsigned char count = input[inIdx++];

        if (count == 0) {
            eobFound = true;
            break;
        }

        if (count > expectedOutputSize - outIdx)
            return corruptedRLE2(output, outputSize);
        memset(*output + outIdx, 0, count);
        outIdx += count;
    }

    if (!eobFound) return corruptedRLE2(output, outputSize);
    if (outIdx != expectedOutputSize) return corruptedRLE2(output, outputSize);
    if (inIdx != inputSize) return corruptedRLE2(output, outputSize);

    *outputSize = outIdx;
    return success;
}
