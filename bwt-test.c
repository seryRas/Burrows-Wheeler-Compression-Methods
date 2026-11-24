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
        .expectedIndex = 6,
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
        .initialIndex = 6,
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
    for (unsigned int i = 0; i < sz; i++)
        buf[i] = (unsigned char)('a' + (i % 3));  // abcabc...
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

// ============================================================================
//  Naive sentinel-BWT reference oracle & targeted RAM-optimised SA-IS tests
// ============================================================================

static const unsigned char* gOracleInput = NULL;
static unsigned int gOracleLen = 0;

static int oracleSuffixCmp(const void* a, const void* b) {
    unsigned int i = *(const unsigned int*)a;
    unsigned int j = *(const unsigned int*)b;
    if (i == j) return 0;
    while (i < gOracleLen && j < gOracleLen) {
        if (gOracleInput[i] != gOracleInput[j]) {
            return (gOracleInput[i] < gOracleInput[j]) ? -1 : 1;
        }
        i++;
        j++;
    }
    // Shorter suffix reaches virtual sentinel first and is strictly smaller
    return (i == gOracleLen) ? -1 : 1;
}

static bool computeNaiveBwt(const unsigned char* input, unsigned int inputSize,
                            unsigned char* expectedOut,
                            unsigned int* expectedIdx) {
    unsigned int saLen = inputSize + 1;
    unsigned int* sa = malloc(sizeof(unsigned int) * (size_t)saLen);
    if (!sa) return false;
    for (unsigned int i = 0; i < saLen; i++) sa[i] = i;

    gOracleInput = input;
    gOracleLen = inputSize;
    qsort(sa, saLen, sizeof(unsigned int), oracleSuffixCmp);

    unsigned int outPos = 0;
    for (unsigned int i = 0; i < saLen; i++) {
        if (sa[i] == 0) {
            *expectedIdx = i;
        } else {
            expectedOut[outPos++] = input[sa[i] - 1];
        }
    }
    free(sa);
    return true;
}

static testResult verifyAgainstOracle(const char* name,
                                      const unsigned char* input,
                                      unsigned int inputSize, bool silent) {
    unsigned char* expectedBwt = malloc(inputSize);
    unsigned char* actualBwt = malloc(inputSize);
    unsigned char* restored = malloc(inputSize);
    unsigned char* inputCopy = malloc(inputSize);
    if (!expectedBwt || !actualBwt || !restored || !inputCopy) {
        free(expectedBwt);
        free(actualBwt);
        free(restored);
        free(inputCopy);
        return error;
    }
    memcpy(inputCopy, input, inputSize);

    unsigned int expectedIdx = 0;
    if (!computeNaiveBwt(inputCopy, inputSize, expectedBwt, &expectedIdx)) {
        free(expectedBwt);
        free(actualBwt);
        free(restored);
        free(inputCopy);
        return error;
    }

    unsigned int actualIdx = 0;
    if (bwtTransform(inputCopy, inputSize, actualBwt, &actualIdx) != success) {
        fprintf(stdout, "FAIL: %s (bwtTransform returned error)\n", name);
        free(expectedBwt);
        free(actualBwt);
        free(restored);
        free(inputCopy);
        return fail;
    }

    if (actualIdx != expectedIdx ||
        memcmp(expectedBwt, actualBwt, inputSize) != 0) {
        fprintf(stdout,
                "FAIL: %s (BWT mismatch: idx got %u expected %u, size %u)\n",
                name, actualIdx, expectedIdx, inputSize);
        free(expectedBwt);
        free(actualBwt);
        free(restored);
        free(inputCopy);
        return fail;
    }

    if (bwtRetransform(actualBwt, inputSize, restored, actualIdx) != success ||
        memcmp(inputCopy, restored, inputSize) != 0) {
        fprintf(stdout, "FAIL: %s (bwtRetransform mismatch, size %u)\n", name,
                inputSize);
        free(expectedBwt);
        free(actualBwt);
        free(restored);
        free(inputCopy);
        return fail;
    }

    if (!silent) fprintf(stdout, "PASS: %s\n", name);
    free(expectedBwt);
    free(actualBwt);
    free(restored);
    free(inputCopy);
    return pass;
}

static testResult runSilentRoundTrip(const char* name,
                                     const unsigned char* input,
                                     unsigned int inputSize) {
    unsigned char* transformed = malloc(inputSize);
    unsigned char* restored = malloc(inputSize);
    unsigned char* inputCopy = malloc(inputSize);
    if (!transformed || !restored || !inputCopy) {
        free(transformed);
        free(restored);
        free(inputCopy);
        return error;
    }
    memcpy(inputCopy, input, inputSize);

    unsigned int bwtIndex = 0;
    if (bwtTransform(inputCopy, inputSize, transformed, &bwtIndex) != success) {
        fprintf(stdout, "FAIL: %s (bwtTransform error)\n", name);
        free(transformed);
        free(restored);
        free(inputCopy);
        return fail;
    }
    if (bwtRetransform(transformed, inputSize, restored, bwtIndex) != success ||
        memcmp(inputCopy, restored, inputSize) != 0) {
        fprintf(stdout, "FAIL: %s (round-trip mismatch, size %u)\n", name,
                inputSize);
        free(transformed);
        free(restored);
        free(inputCopy);
        return fail;
    }

    fprintf(stdout, "PASS: %s\n", name);
    free(transformed);
    free(restored);
    free(inputCopy);
    return pass;
}

testResult exhaustiveBinarySmallTest() {
    const char* name = "Exhaustive binary strings len 1..10 (oracle)";
    unsigned char buf[10];
    for (unsigned int len = 1; len <= 10; len++) {
        unsigned int total = 1u << len;
        for (unsigned int mask = 0; mask < total; mask++) {
            for (unsigned int i = 0; i < len; i++) {
                buf[i] = (unsigned char)((mask >> i) & 1u);
            }
            testResult r = verifyAgainstOracle(name, buf, len, true);
            if (r != pass) return r;
        }
    }
    fprintf(stdout, "PASS: %s\n", name);
    return pass;
}

testResult bitvectorBoundarySizesTest() {
    const char* name = "Bitvector byte-boundary sizes 1..33 (oracle)";
    unsigned char buf[33];
    unsigned int rng = 0x12345678u;
    for (unsigned int len = 1; len <= 33; len++) {
        // Zigzag (maximum LMS density, crossing 8/16/24/32-bit boundaries)
        for (unsigned int i = 0; i < len; i++) {
            buf[i] = (i & 1u) ? (unsigned char)(1u + (i % 3u)) : 200u;
        }
        testResult r = verifyAgainstOracle(name, buf, len, true);
        if (r != pass) return r;

        // Deterministic pseudo-random pattern
        for (unsigned int i = 0; i < len; i++) {
            rng = rng * 1103515245u + 12345u;
            buf[i] = (unsigned char)((rng >> 16) % 5u);
        }
        r = verifyAgainstOracle(name, buf, len, true);
        if (r != pass) return r;
    }
    fprintf(stdout, "PASS: %s\n", name);
    return pass;
}

testResult noInteriorLmsTest() {
    const char* name = "No interior LMS (lmsCount == 1, oracle)";
    unsigned char buf[256];

    // Strictly ascending: S..S, L, $ -> only sentinel is LMS
    for (unsigned int i = 0; i < 256; i++) buf[i] = (unsigned char)i;
    if (verifyAgainstOracle(name, buf, 256, true) != pass) return fail;

    // Strictly descending: L..L, $ -> only sentinel is LMS
    for (unsigned int i = 0; i < 256; i++) buf[i] = (unsigned char)(255u - i);
    if (verifyAgainstOracle(name, buf, 256, true) != pass) return fail;

    // Unimodal (ascending then descending): S..S, L..L, $ -> only sentinel LMS
    for (unsigned int i = 0; i < 128; i++) buf[i] = (unsigned char)i;
    for (unsigned int i = 0; i < 128; i++) buf[128 + i] = (unsigned char)(127u - i);
    if (verifyAgainstOracle(name, buf, 256, true) != pass) return fail;

    fprintf(stdout, "PASS: %s\n", name);
    return pass;
}

testResult singleDiffInRunTest() {
    const char* name = "Single differing byte in uniform run (oracle)";
    const unsigned int sz = 129;
    unsigned char buf[129];
    unsigned int positions[3] = {0, sz / 2, sz - 1};
    unsigned char baseVals[2] = {'a', 'z'};
    unsigned char diffVals[2] = {'z', 'a'};

    for (int v = 0; v < 2; v++) {
        for (int p = 0; p < 3; p++) {
            memset(buf, baseVals[v], sz);
            buf[positions[p]] = diffVals[v];
            if (verifyAgainstOracle(name, buf, sz, true) != pass) return fail;
        }
    }
    fprintf(stdout, "PASS: %s\n", name);
    return pass;
}

testResult nullAndMaxBytesTest() {
    const char* name = "Null (0x00) and max (0xFF) byte boundaries (oracle)";
    unsigned char buf[128];
    for (unsigned int i = 0; i < sizeof(buf); i++) {
        if (i == 0 || i == sizeof(buf) - 1 || (i % 7 == 0)) {
            buf[i] = 0x00;
        } else if (i % 5 == 0) {
            buf[i] = 0xFF;
        } else {
            buf[i] = (unsigned char)(i % 4u);
        }
    }
    return verifyAgainstOracle(name, buf, (unsigned int)sizeof(buf), false);
}

testResult lmsEndingAtSentinelTest() {
    const char* name = "LMS substring ending at sentinel vs interior LMS (oracle)";
    const char* cases[] = {
        "babab",
        "cababcabab",
        "abacabadabacab",
        "dcabdcabdcab",
        "babaababa",
    };
    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        unsigned int len = (unsigned int)strlen(cases[c]);
        if (verifyAgainstOracle(name, (const unsigned char*)cases[c], len,
                                true) != pass) {
            return fail;
        }
    }
    fprintf(stdout, "PASS: %s\n", name);
    return pass;
}

testResult nonRecursiveUniqueLmsTest() {
    const char* name = "All-unique LMS substrings (no recursion path, oracle)";
    // 60 distinct 3-byte blocks (250, x, y) -> 61 unique LMS, skips recursion
    const unsigned int blocks = 60;
    const unsigned int sz = blocks * 3;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int b = 0; b < blocks; b++) {
        buf[b * 3] = 250;
        buf[b * 3 + 1] = (unsigned char)(1u + (b % 30u));
        buf[b * 3 + 2] = (unsigned char)(40u + b);
    }
    testResult r = verifyAgainstOracle(name, buf, sz, false);
    free(buf);
    return r;
}

testResult recursiveTier2FreeSpaceTest() {
    const char* name = "Recursive SA-IS Tier 2 (freeSpace/2 < alpha <= freeSpace, oracle)";
    // 200 blocks of 3 bytes (250, x, y) with 0 < x < y < 250:
    // Each block contributes exactly 1 LMS at offset 1 -> lmsCount = 201,
    // saLen = 601, freeSpace = 601 - 2*201 = 199 (freeSpace/2 = 99).
    // 150 distinct blocks + 50 repeated -> alphabetSize ~ 151 in (99, 199],
    // exercising the zero-heap recomputed bucketBounds branch in recursiveSais.
    const unsigned int blocks = 200;
    const unsigned int uniqueBlocks = 150;
    const unsigned int sz = blocks * 3;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int b = 0; b < blocks; b++) {
        unsigned int id = b % uniqueBlocks;
        buf[b * 3] = 250;
        buf[b * 3 + 1] = (unsigned char)(1u + (id / 10u));
        buf[b * 3 + 2] = (unsigned char)(20u + (id % 10u) + (id / 10u));
    }
    testResult r = verifyAgainstOracle(name, buf, sz, false);
    free(buf);
    return r;
}

testResult recursiveTier3HeapFallbackEvenTest() {
    const char* name = "Recursive SA-IS Tier 3 heap fallback (even saLen, freeSpace=0, oracle)";
    // Odd len = 399 -> saLen = 400, zigzag puts LMS at every odd index
    // 1, 3, ..., 399 -> lmsCount = 200 == saLen/2, freeSpace = 0.
    // Repeated valley values force recursion with alphabetSize > freeSpace.
    const unsigned int sz = 399;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int i = 0; i < sz; i++) {
        if ((i & 1u) == 0) {
            buf[i] = 250;
        } else {
            buf[i] = (unsigned char)(1u + ((i >> 1) % 37u));
        }
    }
    testResult r = verifyAgainstOracle(name, buf, sz, false);
    free(buf);
    return r;
}

testResult recursiveTier3HeapFallbackOddTest() {
    const char* name = "Recursive SA-IS Tier 3 heap fallback (odd saLen, freeSpace=1, oracle)";
    // Even len = 400 -> saLen = 401; zigzag up to 398, then two high bytes so
    // LMS are 1, 3, ..., 397, and sentinel 400 -> lmsCount = 200, freeSpace = 1.
    // Tests sentinel index = 400 -> lmsCount + (400 >> 1) == 400 == saLen - 1.
    const unsigned int sz = 400;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int i = 0; i < sz - 2; i++) {
        if ((i & 1u) == 0) {
            buf[i] = 250;
        } else {
            buf[i] = (unsigned char)(1u + ((i >> 1) % 29u));
        }
    }
    buf[sz - 2] = 250;
    buf[sz - 1] = 250;
    testResult r = verifyAgainstOracle(name, buf, sz, false);
    free(buf);
    return r;
}

testResult nestedMultiLevelHeapFallbackTest() {
    const char* name = "Nested multi-level recursion with tight freeSpace (oracle)";
    // Hierarchically modulated zigzag: forces freeSpace == 0 at level 1 and
    // repeated zigzag structure at deeper levels, stressing heapBucket release
    // before recursion and stacked right-end reducedStr compaction.
    const unsigned int sz = 1023;  // saLen = 1024
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int i = 0; i < sz; i++) {
        if ((i & 1u) == 0) {
            buf[i] = 200;
        } else {
            unsigned int k = i >> 1;
            // Alternate high/low valley pairs to induce zigzag in reducedStr
            unsigned char v = (k & 1u) ? (unsigned char)(10u + (k & 7u))
                                       : (unsigned char)(100u + ((k >> 3) & 3u));
            buf[i] = v;
        }
    }
    testResult r = verifyAgainstOracle(name, buf, sz, false);
    free(buf);
    return r;
}

testResult thueMorseDeepRecursionTest() {
    const char* name = "Thue-Morse 16384B (deep multi-level recursion + Tier 1)";
    const unsigned int sz = 16384;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    for (unsigned int i = 0; i < sz; i++) {
        unsigned int x = i;
        unsigned int parity = 0;
        while (x) {
            parity ^= (x & 1u);
            x >>= 1;
        }
        buf[i] = parity ? 'b' : 'a';
    }
    testResult r = runSilentRoundTrip(name, buf, sz);
    free(buf);
    return r;
}

testResult largeStructuredBlockTest() {
    const char* name = "Large 200 KB mixed structured/random block round-trip";
    const unsigned int sz = 200000;
    unsigned char* buf = malloc(sz);
    if (!buf) return error;
    unsigned int rng = 0xCAFEBABEu;
    for (unsigned int i = 0; i < sz; i++) {
        if (i < 50000) {
            // Repetitive motif with perturbations
            buf[i] = (unsigned char)("abracadabra!0123"[i & 15u] ^
                                     ((i % 257u == 0) ? 1u : 0u));
        } else if (i < 100000) {
            // Maximum-LMS zigzag section
            buf[i] = (i & 1u) ? (unsigned char)((i >> 1) % 53u) : 255u;
        } else if (i < 150000) {
            // Long runs mixed with nulls
            buf[i] = (unsigned char)((i / 17u) % 6u);
        } else {
            // Pseudo-random full-byte range
            rng = rng * 1103515245u + 12345u;
            buf[i] = (unsigned char)(rng >> 16);
        }
    }
    testResult r = runSilentRoundTrip(name, buf, sz);
    free(buf);
    return r;
}

testResult retransformBoundaryIndexTest() {
    const char* name = "Retransform boundary bwtIndex checks (0, 1, n, n+1)";
    unsigned char out[8];
    unsigned char data[] = "abc";
    if (bwtRetransform(data, 3, out, 0) != generalError) {
        fprintf(stdout, "FAIL: %s (bwtIndex=0 should fail)\n", name);
        return fail;
    }
    if (bwtRetransform(data, 3, out, 4) != generalError) {
        fprintf(stdout, "FAIL: %s (bwtIndex=n+1 should fail)\n", name);
        return fail;
    }
    // bwtIndex == 1 ("ab" -> "ba", idx 1)
    unsigned char abTrans[] = "ba";
    if (bwtRetransform(abTrans, 2, out, 1) != success ||
        memcmp(out, "ab", 2) != 0) {
        fprintf(stdout, "FAIL: %s (bwtIndex=1 failed)\n", name);
        return fail;
    }
    // bwtIndex == n ("ba" -> "ab", idx 2)
    unsigned char baTrans[] = "ab";
    if (bwtRetransform(baTrans, 2, out, 2) != success ||
        memcmp(out, "ba", 2) != 0) {
        fprintf(stdout, "FAIL: %s (bwtIndex=n failed)\n", name);
        return fail;
    }
    fprintf(stdout, "PASS: %s\n", name);
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
    counter[exhaustiveBinarySmallTest()]++;
    counter[bitvectorBoundarySizesTest()]++;
    counter[noInteriorLmsTest()]++;
    counter[singleDiffInRunTest()]++;
    counter[nullAndMaxBytesTest()]++;
    counter[lmsEndingAtSentinelTest()]++;
    counter[nonRecursiveUniqueLmsTest()]++;
    counter[recursiveTier2FreeSpaceTest()]++;
    counter[recursiveTier3HeapFallbackEvenTest()]++;
    counter[recursiveTier3HeapFallbackOddTest()]++;
    counter[nestedMultiLevelHeapFallbackTest()]++;
    counter[thueMorseDeepRecursionTest()]++;
    counter[largeStructuredBlockTest()]++;
    counter[retransformBoundaryIndexTest()]++;

    fprintf(stdout, "Tests completed, PASSES: %i, FAILS: %i, ERRORS:%i\n",
            counter[pass], counter[fail], counter[error]);

    return (counter[fail] == 0 && counter[error] == 0) ? 0 : 1;
}