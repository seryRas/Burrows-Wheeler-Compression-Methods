#include <stdlib.h>

#include "errors.h"

typedef struct {
    size_t leftoverBits;
    unsigned char bitAmount;
    size_t outputIdx;
} writerData;

errors writeData(unsigned char size, size_t value, unsigned char* output,
                 writerData* data) {}