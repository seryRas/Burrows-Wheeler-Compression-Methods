#include "BWT.h"

typedef enum {
    pass,
    fail,
    error,
} testResult;

typedef struct {
    const char* name;
    unsigned char* input;
    unsigned int inputSize;
    unsigned char* expectedResult;
    unsigned int expectedIndex;
} transformationCase;

typedef struct {
    const char* name;
    unsigned char* input;
    unsigned int inputSize;
    errors expectedReturn;
} errorCase;

typedef struct {
    const char* name;
    unsigned char* transformed;
    unsigned int inputSize;
    unsigned int initialIndex;
    unsigned char* expectedOutput;
} retransformCase;

typedef struct {
    const char* name;
    unsigned char* input;
    unsigned int inputSize;
} roundTripCase;

testResult runTransformationCase(transformationCase testCase) {
    unsigned char* result = malloc(testCase.inputSize);
    if (result == NULL) return error;

    unsigned int bwtIndex = 0;
    if (bwtTransform(testCase.input, testCase.inputSize, result, &bwtIndex) != success) {
        free(result);
        return error;
    }

    if (memcmp(testCase.expectedResult, result,
               testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.expectedResult, result);
        free(result);
        return fail;
    }

    if (bwtIndex != testCase.expectedIndex) {
        fprintf(stdout, "FAIL: %s, expected index: %u, received: %u\n",
                testCase.name, testCase.expectedIndex, bwtIndex);
        free(result);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(result);
    return pass;
}

testResult runErrorCase(errorCase testCase) {
    unsigned char* result = malloc(testCase.inputSize);
    if (testCase.inputSize > 0 && result == NULL) return error;

    unsigned int bwtIndex = 0;
    if (bwtTransform(testCase.input, testCase.inputSize, result, &bwtIndex) !=
        testCase.expectedReturn) {
        fprintf(stdout, "FAIL: %s, expected return code: %i\n", testCase.name,
                testCase.expectedReturn);
        free(result);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(result);
    return pass;
}

testResult runRetransformCase(retransformCase testCase) {
    unsigned char* transformed = malloc(testCase.inputSize);
    if (transformed == NULL) return error;
    memcpy(transformed, testCase.transformed, testCase.inputSize);
    unsigned char* output = malloc(testCase.inputSize + 1);
    if (output == NULL) {
        free(transformed);
        return error;
    }
    output[testCase.inputSize] = '\0';

    if (bwtRetransform(transformed, testCase.inputSize, output, testCase.initialIndex) != success) {
        free(output);
        free(transformed);
        return error;
    }

    if (memcmp(testCase.expectedOutput, output, testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.expectedOutput, output);
        free(output);
        free(transformed);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(output);
    free(transformed);
    return pass;
}

testResult runRoundTripCase(roundTripCase testCase) {
    unsigned char* transformed = malloc(testCase.inputSize);
    if (transformed == NULL) return error;

    unsigned int bwtIndex = 0;
    if (bwtTransform(testCase.input, testCase.inputSize, transformed, &bwtIndex) !=
        success) {
        free(transformed);
        return error;
    }

    unsigned char* output = malloc(testCase.inputSize + 1);
    if (output == NULL) {
        free(transformed);
        return error;
    }
    output[testCase.inputSize] = '\0';

    if (bwtRetransform(transformed, testCase.inputSize, output, bwtIndex) != success) {
        free(output);
        free(transformed);
        return error;
    }

    if (memcmp(testCase.input, output, testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.input, output);
        free(output);
        free(transformed);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(output);
    free(transformed);
    return pass;
}

testResult runRetransformErrorCase(errorCase testCase) {
    unsigned char* transformed = malloc(testCase.inputSize);
    if (testCase.inputSize > 0 && transformed == NULL) return error;
    memcpy(transformed, testCase.input, testCase.inputSize);
    unsigned char outputPlaceholder[1] = {0};

    if (bwtRetransform(transformed, testCase.inputSize, outputPlaceholder, 0) !=
        testCase.expectedReturn) {
        fprintf(stdout, "FAIL: %s, expected return code: %i\n", testCase.name,
                testCase.expectedReturn);
        free(transformed);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(transformed);
    return pass;
}

testResult transformationTest() {
    unsigned char word[] = "test";
    unsigned char expectedResult[] = "ttes";
    transformationCase testCase = {
        .name = "Transformation test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 4,
    };

    return runTransformationCase(testCase);
}

testResult singleCharacterTest() {
    unsigned char word[] = "x";
    unsigned char expectedResult[] = "x";
    transformationCase testCase = {
        .name = "Single character test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 1,
    };

    return runTransformationCase(testCase);
}

testResult twoCharacterTest() {
    unsigned char word[] = "ab";
    unsigned char expectedResult[] = "ba";
    transformationCase testCase = {
        .name = "Two character test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 1,
    };

    return runTransformationCase(testCase);
}

testResult repeatedCharacterMixTest() {
    unsigned char word[] = "aaba";
    unsigned char expectedResult[] = "abaa";
    transformationCase testCase = {
        .name = "Repeated character mix test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 2,
    };

    return runTransformationCase(testCase);
}

testResult punctuationAndSpaceTest() {
    unsigned char word[] = "a a!";
    unsigned char expectedResult[] = "!aa ";
    transformationCase testCase = {
        .name = "Punctuation and space test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 3,
    };

    return runTransformationCase(testCase);
}

testResult allSameCharactersTest() {
    unsigned char word[] = "aaaaaa";
    unsigned char expectedResult[] = "aaaaaa";
    transformationCase testCase = {
        .name = "All same characters test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 0,
    };

    return runTransformationCase(testCase);
}

testResult retransformKnownCaseTest() {
    unsigned char transformed[] = "atda";
    unsigned char expectedOutput[] = "data";
    retransformCase testCase = {
        .name = "Retransform known case test",
        .transformed = transformed,
        .inputSize = sizeof(transformed) - 1,
        .initialIndex = 3,
        .expectedOutput = expectedOutput,
    };

    return runRetransformCase(testCase);
}

testResult retransformAllSameCharactersTest() {
    unsigned char transformed[] = "aaaaaa";
    unsigned char expectedOutput[] = "aaaaaa";
    retransformCase testCase = {
        .name = "Retransform all same characters test",
        .transformed = transformed,
        .inputSize = sizeof(transformed) - 1,
        .initialIndex = 0,
        .expectedOutput = expectedOutput,
    };

    return runRetransformCase(testCase);
}

testResult roundTripBasicTest() {
    unsigned char word[] = "banana";
    roundTripCase testCase = {
        .name = "Round-trip basic test",
        .input = word,
        .inputSize = sizeof(word) - 1,
    };

    return runRoundTripCase(testCase);
}

testResult roundTripPunctuationTest() {
    unsigned char word[] = "a a!";
    roundTripCase testCase = {
        .name = "Round-trip punctuation test",
        .input = word,
        .inputSize = sizeof(word) - 1,
    };

    return runRoundTripCase(testCase);
}

testResult roundTripAllSameTest() {
    unsigned char word[] = "aaaaaa";
    roundTripCase testCase = {
        .name = "Round-trip all same characters test",
        .input = word,
        .inputSize = sizeof(word) - 1,
    };

    return runRoundTripCase(testCase);
}
testResult roundTripFibonacciTest() {
    unsigned int sz = 2000;
    unsigned char* buf = calloc(sz, 1);
    buf[0] = 'b';
    buf[1] = 'a';
    unsigned int la = 1, lb = 1, pos = 2;
    while (pos < sz) {
        unsigned int copylen = la;
        if (pos + copylen > sz) copylen = sz - pos;
        memcpy(buf + pos, buf + pos - la - lb, copylen);
        pos += copylen;
        unsigned int tmp = la;
        la = la + lb;
        lb = tmp;
    }
    roundTripCase testCase = {
        .name = "Round-trip Fibonacci (highly repetitive) test",
        .input = buf,
        .inputSize = sz,
    };
    testResult res = runRoundTripCase(testCase);
    free(buf);
    return res;
}

testResult roundTripBinaryTest() {
    unsigned int sz = 500;
    unsigned char* buf = malloc(sz);
    for (unsigned int i = 0; i < sz; i++) buf[i] = (i % 2) ? 0xFF : 0x00;
    roundTripCase testCase = {
        .name = "Round-trip binary (0x00 / 0xFF) test",
        .input = buf,
        .inputSize = sz,
    };
    testResult res = runRoundTripCase(testCase);
    free(buf);
    return res;
}

testResult roundTripAscendingTest() {
    unsigned int sz = 256;
    unsigned char* buf = malloc(sz);
    for (unsigned int i = 0; i < sz; i++) buf[i] = (unsigned char)i;
    roundTripCase testCase = {
        .name = "Round-trip ascending test",
        .input = buf,
        .inputSize = sz,
    };
    testResult res = runRoundTripCase(testCase);
    free(buf);
    return res;
}

testResult roundTripAlternatingTest() {
    unsigned int sz = 300;
    unsigned char* buf = malloc(sz);
    for (unsigned int i = 0; i < sz; i++) buf[i] = 'a' + (i % 3);  // abcabc...
    roundTripCase testCase = {
        .name = "Round-trip alternating (abcabc...) test",
        .input = buf,
        .inputSize = sz,
    };
    testResult res = runRoundTripCase(testCase);
    free(buf);
    return res;
}

testResult roundTripRandomTest() {
    unsigned int sz = 5000;
    unsigned char* buf = malloc(sz);
    srand(42);
    for (unsigned int i = 0; i < sz; i++)
        buf[i] = (unsigned char)(rand() % 256);
    roundTripCase testCase = {
        .name = "Round-trip large random data test",
        .input = buf,
        .inputSize = sz,
    };
    testResult res = runRoundTripCase(testCase);
    free(buf);
    return res;
}


testResult transformEmptyInputTest() {
    errorCase testCase = {
        .name = "Transform empty input test",
        .input = (unsigned char*)"",
        .inputSize = 0,
        .expectedReturn = emptyInput
    };
    return runErrorCase(testCase);
}

testResult retransformEmptyInputTest() {
    errorCase testCase = {
        .name = "Retransform empty input test",
        .input = (unsigned char*)"",
        .inputSize = 0,
        .expectedReturn = emptyInput
    };
    return runRetransformErrorCase(testCase);
}

testResult retransformOutOfBoundsIndexTest() {
    errorCase testCase = {
        .name = "Retransform out-of-bounds index test",
        .input = (unsigned char*)"abc",
        .inputSize = 3,
        .expectedReturn = generalError
    };
    unsigned char output[10];
    if (bwtRetransform(testCase.input, testCase.inputSize, output, 4) != generalError) {
        fprintf(stdout, "FAIL: %s\n", testCase.name);
        return fail;
    }
    fprintf(stdout, "PASS: %s\n", testCase.name);
    return pass;
}

int main() {
    int counter[3] = {0};
    counter[transformationTest()]++;
    counter[singleCharacterTest()]++;
    counter[twoCharacterTest()]++;
    counter[repeatedCharacterMixTest()]++;
    counter[punctuationAndSpaceTest()]++;
    counter[allSameCharactersTest()]++;
    counter[retransformKnownCaseTest()]++;
    counter[retransformAllSameCharactersTest()]++;
    counter[roundTripBasicTest()]++;
    counter[roundTripPunctuationTest()]++;
    counter[roundTripAllSameTest()]++;
    counter[roundTripFibonacciTest()]++;
    counter[roundTripBinaryTest()]++;
    counter[roundTripAscendingTest()]++;
    counter[roundTripAlternatingTest()]++;
    counter[roundTripRandomTest()]++;
    counter[transformEmptyInputTest()]++;
    counter[retransformEmptyInputTest()]++;
    counter[retransformOutOfBoundsIndexTest()]++;

    fprintf(stdout, "Tests completed, PASSES: %i, FAILS: %i, ERRORS:%i\n",
            counter[pass], counter[fail], counter[error]);

    return (counter[fail] == 0 && counter[error] == 0) ? 0 : 1;
}