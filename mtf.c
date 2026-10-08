#include "mtf.h"
#include <string.h>

errors MTFencode(unsigned char* input, unsigned int inputSize,
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

errors MTFdecode(unsigned char* input, unsigned int inputSize,
                 unsigned char* output, unsigned char* dictionary,
                 unsigned int dictionarySize) {
    unsigned int index = 0;
    while (index < inputSize) {
        unsigned int i = input[index];
        if (i >= dictionarySize) return corruptedData;

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
