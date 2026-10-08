#ifndef MTF_H
#define MTF_H

#include "errors.h"

errors MTFencode(unsigned char* input, unsigned int inputSize,
                 unsigned char* output, unsigned char* dictionary,
                 unsigned int dictionarySize);

errors MTFdecode(unsigned char* input, unsigned int inputSize,
                 unsigned char* output, unsigned char* dictionary,
                 unsigned int dictionarySize);

#endif
