#include "BWT.h"

typedef enum {
    pass,
    fail,
    error,
} testResult;

typedef struct {
    const char* name;
    unsigned char* input;
    size_t inputSize;
    unsigned char* expectedResult;
    size_t expectedIndex;
} transformationCase;

typedef struct {
    const char* name;
    unsigned char* input;
    size_t inputSize;
    errors expectedReturn;
} errorCase;

testResult runTransformationCase(transformationCase testCase) {
    bwt_out result = {.size = testCase.inputSize};
    if ((result.data = malloc(testCase.inputSize + 1)) == NULL) return error;
    result.data[testCase.inputSize] = '\0';

    if (bwtTransform(testCase.input, &result) != succes) {
        free(result.data);
        return error;
    }

    if (memcmp(testCase.expectedResult, result.data, testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.expectedResult, result.data);
        free(result.data);
        return fail;
    }

    if (result.initialIndex != testCase.expectedIndex) {
        fprintf(stdout, "FAIL: %s, expected index: %lu, received: %lu\n",
                testCase.name, testCase.expectedIndex, result.initialIndex);
        free(result.data);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(result.data);
    return pass;
}

testResult runErrorCase(errorCase testCase) {
    bwt_out result = {.size = testCase.inputSize};
    result.data = malloc(testCase.inputSize + 1);
    if (testCase.inputSize > 0 && result.data == NULL) return error;

    if (bwtTransform(testCase.input, &result) != testCase.expectedReturn) {
        fprintf(stdout, "FAIL: %s, expected return code: %i\n", testCase.name,
                testCase.expectedReturn);
        free(result.data);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(result.data);
    return pass;
}

testResult transformationTest(void) {
    unsigned char word[] = "data";
    unsigned char expectedResult[] = "tdaa";
    transformationCase testCase = {
        .name = "Transformation test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 2,
    };

    return runTransformationCase(testCase);
}

testResult singleCharacterTest(void) {
    unsigned char word[] = "x";
    unsigned char expectedResult[] = "x";
    transformationCase testCase = {
        .name = "Single character test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 0,
    };

    return runTransformationCase(testCase);
}

testResult twoCharacterTest(void) {
    unsigned char word[] = "ab";
    unsigned char expectedResult[] = "ba";
    transformationCase testCase = {
        .name = "Two character test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 0,
    };

    return runTransformationCase(testCase);
}

testResult repeatedCharacterMixTest(void) {
    unsigned char word[] = "aaba";
    unsigned char expectedResult[] = "baaa";
    transformationCase testCase = {
        .name = "Repeated character mix test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 1,
    };

    return runTransformationCase(testCase);
}

testResult punctuationAndSpaceTest(void) {
    unsigned char word[] = "a a!";
    unsigned char expectedResult[] = "aa! ";
    transformationCase testCase = {
        .name = "Punctuation and space test",
        .input = word,
        .inputSize = sizeof(word) - 1,
        .expectedResult = expectedResult,
        .expectedIndex = 2,
    };

    return runTransformationCase(testCase);
}

testResult emptyInputTest(void) {
    unsigned char word[] = "";
    errorCase testCase = {
        .name = "Empty input test",
        .input = word,
        .inputSize = 0,
        .expectedReturn = emptyInput,
    };

    return runErrorCase(testCase);
}

testResult allSameCharactersTest(void) {
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

int main(void) {
    int counter[3] = {0};
    counter[transformationTest()]++;
    counter[singleCharacterTest()]++;
    counter[twoCharacterTest()]++;
    counter[repeatedCharacterMixTest()]++;
    counter[punctuationAndSpaceTest()]++;
    counter[emptyInputTest()]++;
    counter[allSameCharactersTest()]++;

    fprintf(stdout, "Tests completed, PASSES: %i, FAILS: %i, ERRORS:%i\n",
            counter[pass], counter[fail], counter[error]);

    return counter[fail] == 0 && counter[error] == 0 ? 0 : 1;
}