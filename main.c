#include "BWT.h"



int main() {
    char arr[] = "pernik";

    bwt_out out = {.size = 6};
    bwt_transform((unsigned char *)arr, &out);
    return 0;
}