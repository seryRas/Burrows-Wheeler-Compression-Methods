#include "BWT.h"

int main() {
    char arr[] =
        "ppppernikajshdfkahjafsdjkhaskjflhasds;dhasjfnaaasfdasfjhafhdsjhsd;jf";

    bwt_out out = {.size = sizeof(arr)};
    bwtTransform((unsigned char*)arr, &out);
    return 0;
}