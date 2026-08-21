#include "bzip2.h"


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

    bwt_out input;
    
}

