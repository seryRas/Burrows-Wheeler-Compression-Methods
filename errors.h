#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    success = 0,
    mallocErr,
    emptyInput,
    fileErr,
    generalError,
    sizeTooBig,
    corruptedData,
} errors;

#endif
