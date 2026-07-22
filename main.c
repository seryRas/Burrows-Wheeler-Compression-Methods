#include "BWT.h"


int main(int argc, char **argv) {
    FILE *testInput = fopen(argv[1], "r");
    char c;
    size_t sizeArr = 0x4FFFFFF;
    unsigned char *arr = malloc(sizeArr);
    size_t i = 0;

    while((c = getc(testInput)) != EOF) {
        arr[i++] = c;
        if(i >= sizeArr) {
            fprintf(stderr, "File too big\n");
            return 1;
        }
    }
    


    bwt_out out = {.size = i - 1};
    if ((out.data = malloc(i - 1)) == NULL) return mallocErr;
    bwtTransform((unsigned char*)arr, &out);
    for (unsigned int i = 0; i < i - 1; i++)
        printf("%c", out.data[i]);
    printf("\n");
    unsigned char* output = malloc(i);
    bwtRetransform(&out, output);
    fprintf(stdout, "%s\n", output);
    return 0;
}
