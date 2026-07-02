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

typedef struct {
    const char* name;
    unsigned char* transformed;
    size_t inputSize;
    size_t initialIndex;
    unsigned char* expectedOutput;
} retransformCase;

typedef struct {
    const char* name;
    unsigned char* input;
    size_t inputSize;
} roundTripCase;

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

testResult runRetransformCase(retransformCase testCase) {
    bwt_out transformed = {
        .data = testCase.transformed,
        .size = testCase.inputSize,
        .initialIndex = testCase.initialIndex,
    };
    unsigned char* output = malloc(testCase.inputSize + 1);
    if (output == NULL) return error;
    output[testCase.inputSize] = '\0';

    if (bwtRetransform(&transformed, output) != succes) {
        free(output);
        return error;
    }

    if (memcmp(testCase.expectedOutput, output, testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.expectedOutput, output);
        free(output);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(output);
    return pass;
}

testResult runRoundTripCase(roundTripCase testCase) {
    bwt_out transformed = {.size = testCase.inputSize};
    transformed.data = malloc(testCase.inputSize + 1);
    if (transformed.data == NULL) return error;
    transformed.data[testCase.inputSize] = '\0';

    if (bwtTransform(testCase.input, &transformed) != succes) {
        free(transformed.data);
        return error;
    }

    unsigned char* output = malloc(testCase.inputSize + 1);
    if (output == NULL) {
        free(transformed.data);
        return error;
    }
    output[testCase.inputSize] = '\0';

    if (bwtRetransform(&transformed, output) != succes) {
        free(output);
        free(transformed.data);
        return error;
    }

    if (memcmp(testCase.input, output, testCase.inputSize) != 0) {
        fprintf(stdout, "FAIL: %s, expected: %s, received: %s\n", testCase.name,
                testCase.input, output);
        free(output);
        free(transformed.data);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    free(output);
    free(transformed.data);
    return pass;
}

testResult runRetransformErrorCase(errorCase testCase) {
    bwt_out transformed = {
        .data = testCase.input,
        .size = testCase.inputSize,
        .initialIndex = 0,
    };
    unsigned char outputPlaceholder[1] = {0};

    if (bwtRetransform(&transformed, outputPlaceholder) !=
        testCase.expectedReturn) {
        fprintf(stdout, "FAIL: %s, expected return code: %i\n", testCase.name,
                testCase.expectedReturn);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", testCase.name);
    return pass;
}

testResult transformationTest() {
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

testResult singleCharacterTest() {
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

testResult twoCharacterTest() {
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

testResult repeatedCharacterMixTest() {
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

testResult punctuationAndSpaceTest() {
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
    unsigned char transformed[] = "tdaa";
    unsigned char expectedOutput[] = "data";
    retransformCase testCase = {
        .name = "Retransform known case test",
        .transformed = transformed,
        .inputSize = sizeof(transformed) - 1,
        .initialIndex = 2,
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