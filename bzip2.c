#include "bzip2.h"

int RLE1encode(unsigned char* input, unsigned int inputSize,
               unsigned char** output, unsigned int* outputSize) {
    if (inputSize == 0) return emptyInput;

    *output = malloc(inputSize + (inputSize / 4) + 1);
    if (!(*output)) return mallocErr;

    int count = 1;
    int index = 0;
    int outputIndex = 0;

    (*output)[outputIndex++] = input[index];
    unsigned char lastChar = input[index++];

    while (index != inputSize) {
        if (input[index] == lastChar) {
            count++;
            if (count == UCHAR_MAX + SEQ_MAXLEN) {
                (*output)[outputIndex++] = count - SEQ_MAXLEN;
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
                (*output)[outputIndex++] = count - SEQ_MAXLEN;
            }
            lastChar = input[index];
            count = 1;
        }

        (*output)[outputIndex++] = input[index++];
    }

    if (count >= SEQ_MAXLEN) {
        (*output)[outputIndex++] = count - SEQ_MAXLEN;
    }

    (*outputSize) = outputIndex;
    return success;
}

int RLE1decode(RLE1info* data, unsigned char** output, unsigned int* outputSize,
               unsigned int chunkSize) {
    if (chunkSize == 0) {
        *outputSize = 0;
        *output = NULL;
        return success;
    }

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

int MTFencode(unsigned char* input, unsigned int inputSize,
              unsigned char* output, unsigned char* dictionary,
              unsigned int dictionarySize) {
    unsigned int index = 0;
    while (index < inputSize) {
        unsigned int i = 0;
        while (i < dictionarySize && dictionary[i] != input[index]) {
            i++;
        }
        if (i >= dictionarySize) return generalError;

        output[index] = (unsigned char)i;

        if (i > 0) {
            unsigned char c = input[index];
            memmove(dictionary + 1, dictionary, i);
            dictionary[0] = c;
        }
        index++;
    }
    return success;
}

int MTFdecode(unsigned char* input, unsigned int inputSize,
              unsigned char* output, unsigned char* dictionary,
              unsigned int dictionarySize) {
    unsigned int index = 0;
    while (index < inputSize) {
        unsigned int i = input[index];
        if (i >= dictionarySize) return generalError;

        unsigned char c = dictionary[i];
        output[index] = c;

        if (i > 0) {
            memmove(dictionary + 1, dictionary, i);
            dictionary[0] = c;
        }
        index++;
    }
    return success;
}

int RLE2encode(unsigned char* input, unsigned int inputSize,
               unsigned char** output, unsigned int* outputSize) {
    if (inputSize == 0) return emptyInput;

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
                unsigned char chunk =
                    (zeroCount > 255) ? 255 : (unsigned char)zeroCount;
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

int RLE2decode(unsigned char* input, unsigned int inputSize,
               unsigned char** output, unsigned int* outputSize,
               unsigned int expectedOutputSize) {
    if (inputSize == 0) return emptyInput;

    *output = malloc(expectedOutputSize);
    if (!(*output)) return mallocErr;

    unsigned int inIdx = 0;
    unsigned int outIdx = 0;

    while (inIdx < inputSize) {
        unsigned char c = input[inIdx++];

        if (c == 0) {
            if (inIdx >= inputSize) return generalError;

            unsigned char count = input[inIdx++];

            if (count == 0) {
                break;
            }

            for (int i = 0; i < count; i++) {
                if (outIdx < expectedOutputSize) {
                    (*output)[outIdx++] = 0;
                }
            }
        } else {
            if (outIdx < expectedOutputSize) {
                (*output)[outIdx++] = c;
            }
        }
    }

    *outputSize = outIdx;
    return success;
}

int main(int argc, char** argv) {
    FILE* inputFile;
    if (argc < 2)
        inputFile = stdin;
    else
        inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        fprintf(stderr, "Failed to open input File\n");
        return fileErr;
    }
}
