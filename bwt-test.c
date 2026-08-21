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
    unsigned char* result = malloc(testCase.inputSize + BWT_HEADER_SIZE);
    if (result == NULL) return error;

    if (bwtTransform(testCase.input, testCase.inputSize, result) != success) {
        free(result);
        return error;
    }

    if (memcmp(testCase.expectedResult, result + BWT_HEADER_SIZE,
               testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.expectedResult, result + BWT_HEADER_SIZE);
        free(result);
        return fail;
    }

    unsigned int initialIndex = 0;
    memcpy(&initialIndex, result + 1, sizeof(unsigned int));
    if (initialIndex != testCase.expectedIndex) {
        fprintf(stdout, "FAIL: %s, expected index: %u, received: %u\n",
                testCase.name, testCase.expectedIndex, initialIndex);
        free(result);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(result);
    return pass;
}

testResult runErrorCase(errorCase testCase) {
    unsigned char* result = malloc(testCase.inputSize + BWT_HEADER_SIZE);
    if (testCase.inputSize > 0 && result == NULL) return error;

    if (bwtTransform(testCase.input, testCase.inputSize, result) !=
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
    unsigned char* transformed = malloc(testCase.inputSize + BWT_HEADER_SIZE);
    if (transformed == NULL) return error;
    transformed[0] = 0;
    memcpy(transformed + 1, &testCase.initialIndex, sizeof(unsigned int));
    memcpy(transformed + BWT_HEADER_SIZE, testCase.transformed,
           testCase.inputSize);
    unsigned char* output = malloc(testCase.inputSize + 1);
    if (output == NULL) {
        free(transformed);
        return error;
    }
    output[testCase.inputSize] = '\0';

    if (bwtRetransform(transformed, testCase.inputSize, output) != success) {
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
    unsigned char* transformed = malloc(testCase.inputSize + BWT_HEADER_SIZE);
    if (transformed == NULL) return error;

    if (bwtTransform(testCase.input, testCase.inputSize, transformed) !=
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

    if (bwtRetransform(transformed, testCase.inputSize, output) != success) {
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
    unsigned char* transformed = malloc(testCase.inputSize + BWT_HEADER_SIZE);
    if (testCase.inputSize > 0 && transformed == NULL) return error;
    transformed[0] = 0;
    memset(transformed + 1, 0, sizeof(unsigned int));
    memcpy(transformed + BWT_HEADER_SIZE, testCase.input, testCase.inputSize);
    unsigned char outputPlaceholder[1] = {0};

    if (bwtRetransform(transformed, testCase.inputSize, outputPlaceholder) !=
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

testResult emptyInputTest() {
    unsigned char word[] = "";
    errorCase testCase = {
        .name = "Empty input test",
        .input = word,
        .inputSize = 0,
        .expectedReturn = emptyInput,
    };

    return runErrorCase(testCase);
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

testResult retransformEmptyInputTest() {
    unsigned char transformed[] = "";
    errorCase testCase = {
        .name = "Retransform empty input test",
        .input = transformed,
        .inputSize = 0,
        .expectedReturn = emptyInput,
    };

    return runRetransformErrorCase(testCase);
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
int main() {
    int counter[3] = {0};
    counter[transformationTest()]++;
    counter[singleCharacterTest()]++;
    counter[twoCharacterTest()]++;
    counter[repeatedCharacterMixTest()]++;
    counter[punctuationAndSpaceTest()]++;
    counter[emptyInputTest()]++;
    counter[allSameCharactersTest()]++;
    counter[retransformKnownCaseTest()]++;
    counter[retransformAllSameCharactersTest()]++;
    counter[retransformEmptyInputTest()]++;
    counter[roundTripBasicTest()]++;
    counter[roundTripPunctuationTest()]++;
    counter[roundTripAllSameTest()]++;

    fprintf(stdout, "Tests completed, PASSES: %i, FAILS: %i, ERRORS:%i\n",
            counter[pass], counter[fail], counter[error]);

    return (counter[fail] == 0 && counter[error] == 0) ? 0 : 1;
}