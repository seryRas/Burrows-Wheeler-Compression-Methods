#include "BWT.h"

int main() {
    char arr[] = "banana";

    bwt_out out = {.size = sizeof(arr) - 1};
    if ((out.data = malloc(sizeof(arr) - 1)) == NULL) return mallocErr;
    bwtTransform((unsigned char*)arr, &out);
    for (unsigned int i = 0; i < sizeof(arr) - 1; i++)
        printf("%c", out.data[i]);
    printf("\n");
    unsigned char* output = malloc(sizeof(arr));
    bwtRetransform(&out, output);
    printf("%s\n", output);
    return 0;
}
