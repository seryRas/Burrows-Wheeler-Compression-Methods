#include "bzip2.h"

int RLE1encode(unsigned char *input, unsigned int inputSize, unsigned char **output, unsigned int *outputSize) {
    *output = malloc(sizeof(unsigned char) * inputSize * 1.25);
    if(!(*output)) return mallocErr;
    int count = 1;
    int index = 0;
    int outputIndex = 0;
    (*output)[outputIndex++] = input[index];
    unsigned char lastChar = input[index++];
    while(index != inputSize) {
        if(input[index] == lastChar) {
            count++;
            if(count == UCHAR_MAX + SEQ_MAXLEN) {
                (*output)[outputIndex++] = count - SEQ_MAXLEN;
                count = 0;
            }
            if(count > SEQ_MAXLEN) {
                index++;
                continue;
            }
        }
        else {
            if(count >= SEQ_MAXLEN) {
                (*output)[outputIndex++] = count - SEQ_MAXLEN;
            }
            lastChar = input[index];
            count = 1;
        }

        (*output)[outputIndex++] = input[index++];
    }

    if(count >= SEQ_MAXLEN) {
        output[outputIndex++] = count - SEQ_MAXLEN;
    }

    (*outputSize) = outputIndex;
    return success;
}


int RLE1decode(unsigned char *input, unsigned int inputSize, unsigned char **output, unsigned int *outputSize) {
    
}


void MTF(char *input) {

}



int main(int argc, char **argv) {
    FILE *inputFile;
    if(argc < 2) inputFile = stdin;
    else inputFile = fopen(argv[1], "r");
    if(!inputFile) {
        fprintf(stderr, "Failed to open input File\n");
        return fileErr;
    }

    
    
}

