#include "BWT.h"

#define SEQ_MAXLEN 4

typedef struct {
    unsigned char* next;
    unsigned char remainingToAdd;
    unsigned char lastChar;
    unsigned char charCount;
    bool done;
    unsigned int remainingSize;
} RLE1info;
