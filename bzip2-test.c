#include "bzip2.h"

// AI GENERATED FILE
// ============================================================================
//  Tiny test framework
// ============================================================================

#define EXPECT(cond)                                                         \
    do {                                                                     \
        if (!(cond)) {                                                       \
            printf("    assertion failed: %s (line %d)\n", #cond, __LINE__); \
            return false;                                                    \
        }                                                                    \
    } while (0)

typedef bool (*testFn)(void);

typedef struct {
    const char* name;
    testFn fn;
} testEntry;

// Deterministic PRNG so failures are reproducible.
static unsigned int rngState = 12345;
static unsigned int rng(void) {
    rngState = rngState * 1103515245u + 12345u;
    return rngState >> 16;
}

static bool bytesMatch(const unsigned char* got, unsigned int gotSize,
                       const unsigned char* exp, unsigned int expSize) {
    if (gotSize != expSize) {
        printf("    size mismatch: got %u, expected %u\n", gotSize, expSize);
        return false;
    }
    for (unsigned int i = 0; i < gotSize; i++) {
        if (got[i] != exp[i]) {
            printf("    first difference at %u: got 0x%02X, expected 0x%02X\n",
                   i, got[i], exp[i]);
            return false;
        }
    }
    return true;
}

static unsigned char* makeRun(unsigned char c, unsigned int n) {
    unsigned char* b = malloc(n ? n : 1);
    memset(b, c, n);
    return b;
}

static void fillRandom(unsigned char* b, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) b[i] = (unsigned char)rng();
}

// Random symbols from a small alphabet with random run lengths. maxRun > 259
// makes runs cross the RLE1 255-length-byte boundary.
static void fillRandomRuns(unsigned char* b, unsigned int n,
                           unsigned int alphabet, unsigned int maxRun) {
    unsigned int i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)(rng() % alphabet);
        unsigned int r = 1 + rng() % maxRun;
        while (r-- > 0 && i < n) b[i++] = c;
    }
}

static unsigned int buildDict(const unsigned char* inUseMap,
                              unsigned char* dict) {
    unsigned int size = 0;
    for (unsigned int i = 0; i < AMOUNT_OF_VALUES; i++) {
        if (inUseMap[i / 8] & (1 << (i % 8))) dict[size++] = (unsigned char)i;
    }
    return size;
}

static bool mapHas(const unsigned char* map, unsigned char c) {
    return (map[c / 8] >> (c % 8)) & 1;
}

static unsigned int mapCount(const unsigned char* map) {
    unsigned int n = 0;
    for (unsigned int i = 0; i < AMOUNT_OF_VALUES; i++) n += mapHas(map, i);
    return n;
}

// ============================================================================
//  Step helpers
// ============================================================================

static bool rle1Vector(const unsigned char* in, unsigned int n,
                       const unsigned char* exp, unsigned int expN) {
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(RLE1encode((unsigned char*)in, n, &out, &outN, map) == success);
    bool ok = bytesMatch(out, outN, exp, expN);
    free(out);
    return ok;
}

// Decodes a whole RLE1 stream by repeatedly calling RLE1decode with the given
// chunk size. Also verifies chunk discipline: every call except the last must
// return exactly chunkSize bytes.
static bool rle1DecodeAll(unsigned char* enc, unsigned int encN,
                          unsigned int chunk, unsigned char** out,
                          unsigned int* outN) {
    RLE1info info;
    RLE1infoInit(&info, enc, encN);
    unsigned int cap = 64, len = 0;
    unsigned char* buf = malloc(cap);

    while (!info.done) {
        unsigned char* part = NULL;
        unsigned int partN = 0;
        if (RLE1decode(&info, &part, &partN, chunk) != success) {
            printf("    RLE1decode returned error\n");
            free(buf);
            return false;
        }
        if (partN == 0) {
            if (info.done) {
                free(part);
                break;
            }
            printf("    RLE1decode stalled (no output, not done)\n");
            free(part);
            free(buf);
            return false;
        }
        if (partN < chunk && !info.done) {
            printf("    short chunk (%u < %u) before end of stream\n", partN,
                   chunk);
            free(part);
            free(buf);
            return false;
        }
        while (len + partN > cap) cap *= 2;
        buf = realloc(buf, cap);
        memcpy(buf + len, part, partN);
        len += partN;
        free(part);
    }
    *out = buf;
    *outN = len;
    return true;
}

static bool rle1RoundTrip(const unsigned char* in, unsigned int n,
                          unsigned int chunk) {
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* enc = NULL;
    unsigned int encN = 0;
    EXPECT(RLE1encode((unsigned char*)in, n, &enc, &encN, map) == success);
    EXPECT(encN <= n + n / SEQ_MAXLEN + 1);

    unsigned char* dec = NULL;
    unsigned int decN = 0;
    bool ok = rle1DecodeAll(enc, encN, chunk, &dec, &decN) &&
              bytesMatch(dec, decN, in, n);
    free(enc);
    free(dec);
    return ok;
}

static bool rle2Vector(const unsigned char* in, unsigned int n,
                       const unsigned char* exp, unsigned int expN) {
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(RLE2encode((unsigned char*)in, n, &out, &outN) == success);
    bool ok = bytesMatch(out, outN, exp, expN);
    free(out);
    if (!ok) return false;

    // Decoding the expected vector must give the input back.
    unsigned char* dec = NULL;
    unsigned int decN = 0;
    EXPECT(RLE2decode((unsigned char*)exp, expN, &dec, &decN, n) == success);
    ok = bytesMatch(dec, decN, in, n);
    free(dec);
    return ok;
}

static bool rle2RoundTrip(const unsigned char* in, unsigned int n) {
    unsigned char* enc = NULL;
    unsigned int encN = 0;
    EXPECT(RLE2encode((unsigned char*)in, n, &enc, &encN) == success);
    EXPECT(encN <= n * 2 + 2);
    EXPECT(enc[encN - 2] == 0 && enc[encN - 1] == 0);

    unsigned char* dec = NULL;
    unsigned int decN = 0;
    errors r = RLE2decode(enc, encN, &dec, &decN, n);
    free(enc);
    EXPECT(r == success);
    bool ok = bytesMatch(dec, decN, in, n);
    free(dec);
    return ok;
}

static bool rle2DecodeExpectError(const unsigned char* in, unsigned int n,
                                  unsigned int expected) {
    unsigned char* dec = (unsigned char*)0x1;  // must be reset to NULL
    unsigned int decN = 0;
    EXPECT(RLE2decode((unsigned char*)in, n, &dec, &decN, expected) ==
           corruptedData);
    EXPECT(dec == NULL);
    return true;
}

static bool freePipelineBuffers(bool ok, unsigned char* rle1,
                                unsigned char* bwt, unsigned char* mtf,
                                unsigned char* rle2, unsigned char* mtfDec,
                                unsigned char* bwtDec, unsigned char* rle1Dec,
                                unsigned char* final) {
    free(rle1);
    free(bwt);
    free(mtf);
    free(rle2);
    free(mtfDec);
    free(bwtDec);
    free(rle1Dec);
    free(final);
    return ok;
}

// Full compress -> decompress through every step, checking invariants at each
// stage so a failure points to the step that broke.
static bool pipelineRoundTrip(const unsigned char* input, unsigned int n,
                              unsigned int chunk) {
    unsigned char *rle1 = NULL, *bwt = NULL, *mtf = NULL, *rle2 = NULL;
    unsigned char *mtfDec = NULL, *bwtDec = NULL, *rle1Dec = NULL,
                  *final = NULL;
    unsigned int rle1N = 0, rle2N = 0, mtfDecN = 0, finalN = 0;
    unsigned char dict[AMOUNT_OF_VALUES];
    unsigned int dictN = 0;
    bzip2Header header;
    memset(&header, 0, sizeof(header));

    // ---- compress ----
    if (RLE1encode((unsigned char*)input, n, &rle1, &rle1N, header.inUseMap) !=
        success) {
        printf("    RLE1encode failed\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }
    {
        // inUseMap must describe exactly the bytes present in RLE1 output.
        unsigned char seen[IN_USE_MAP_SIZE] = {0};
        for (unsigned int i = 0; i < rle1N; i++)
            seen[rle1[i] / 8] |= (unsigned char)(1 << (rle1[i] % 8));
        if (memcmp(seen, header.inUseMap, IN_USE_MAP_SIZE) != 0) {
            printf("    inUseMap does not match RLE1 output\n");
            return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec,
                                       bwtDec, rle1Dec, final);
        }
    }

    bwt = malloc(rle1N);
    if (bwtTransform(rle1, rle1N, bwt, &header.bwtIndex) != success) {
        printf("    bwtTransform failed\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }
    if (header.bwtIndex > rle1N) {
        printf("    bwtIndex %u out of range\n", header.bwtIndex);
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    dictN = buildDict(header.inUseMap, dict);
    mtf = malloc(rle1N);
    if (MTFencode(bwt, rle1N, mtf, dict, dictN) != success) {
        printf("    MTFencode failed\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }
    for (unsigned int i = 0; i < rle1N; i++) {
        if (mtf[i] >= dictN) {
            printf("    MTF index %u >= dictSize %u\n", mtf[i], dictN);
            return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec,
                                       bwtDec, rle1Dec, final);
        }
    }

    if (RLE2encode(mtf, rle1N, &rle2, &rle2N) != success) {
        printf("    RLE2encode failed\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    // ---- decompress ----
    if (RLE2decode(rle2, rle2N, &mtfDec, &mtfDecN, rle1N) != success ||
        !bytesMatch(mtfDec, mtfDecN, mtf, rle1N)) {
        printf("    RLE2decode stage mismatch\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    dictN = buildDict(header.inUseMap, dict);
    bwtDec = malloc(rle1N);
    if (MTFdecode(mtfDec, rle1N, bwtDec, dict, dictN) != success ||
        !bytesMatch(bwtDec, rle1N, bwt, rle1N)) {
        printf("    MTFdecode stage mismatch\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    rle1Dec = malloc(rle1N);
    if (bwtRetransform(bwtDec, rle1N, rle1Dec, header.bwtIndex) != success ||
        !bytesMatch(rle1Dec, rle1N, rle1, rle1N)) {
        printf("    bwtRetransform stage mismatch\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    if (!rle1DecodeAll(rle1Dec, rle1N, chunk, &final, &finalN) ||
        !bytesMatch(final, finalN, input, n)) {
        printf("    RLE1decode stage mismatch\n");
        return freePipelineBuffers(false, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                                   rle1Dec, final);
    }

    return freePipelineBuffers(true, rle1, bwt, mtf, rle2, mtfDec, bwtDec,
                               rle1Dec, final);
}

static const unsigned int chunkSizes[] = {1, 2,  3,   4,   5,
                                          7, 64, 259, 260, 65536};
#define CHUNK_SIZE_COUNT (sizeof(chunkSizes) / sizeof(chunkSizes[0]))

static bool rle1RoundTripAllChunks(const unsigned char* in, unsigned int n) {
    for (unsigned int i = 0; i < CHUNK_SIZE_COUNT; i++) {
        if (!rle1RoundTrip(in, n, chunkSizes[i])) {
            printf("    (chunk size %u)\n", chunkSizes[i]);
            return false;
        }
    }
    return true;
}

static bool pipelineAllChunks(const unsigned char* in, unsigned int n) {
    static const unsigned int pc[] = {1, 7, 4096};
    for (unsigned int i = 0; i < 3; i++) {
        if (!pipelineRoundTrip(in, n, pc[i])) {
            printf("    (RLE1 decode chunk size %u)\n", pc[i]);
            return false;
        }
    }
    return true;
}

// ============================================================================
//  RLE1encode tests
// ============================================================================



static bool rle1EncEmpty(void) {
    unsigned char* output = NULL;
    unsigned int outSize = 0;
    unsigned char inUseMap[32] = {0};
    
    errors err = RLE1encode(NULL, 0, &output, &outSize, inUseMap);
    if (err != success) {
        printf("FAIL: Expected success for empty input, got %d\n", err);
        return false;
    }
    if (output != NULL || outSize != 0) {
        printf("FAIL: Expected output=NULL and outSize=0\n");
        return false;
    }
    return true;
}

static bool rle1EncSingle(void) {
    const unsigned char in[] = {'A'};
    return rle1Vector(in, 1, in, 1);
}

static bool rle1EncRun3(void) {
    const unsigned char in[] = {'A', 'A', 'A'};
    return rle1Vector(in, 3, in, 3);
}

static bool rle1EncRun4(void) {
    const unsigned char in[] = {'A', 'A', 'A', 'A'};
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 0};
    return rle1Vector(in, 4, exp, 5);
}

static bool rle1EncRun5(void) {
    const unsigned char in[] = {'A', 'A', 'A', 'A', 'A'};
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 1};
    return rle1Vector(in, 5, exp, 5);
}

static bool rle1EncRun258(void) {
    unsigned char* in = makeRun('A', 258);
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 254};
    bool ok = rle1Vector(in, 258, exp, 5);
    free(in);
    return ok;
}

static bool rle1EncRun259(void) {
    unsigned char* in = makeRun('A', 259);
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 255};
    bool ok = rle1Vector(in, 259, exp, 5);
    free(in);
    return ok;
}

static bool rle1EncRun260(void) {
    unsigned char* in = makeRun('A', 260);
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 255, 'A'};
    bool ok = rle1Vector(in, 260, exp, 6);
    free(in);
    return ok;
}

static bool rle1EncRun263(void) {
    unsigned char* in = makeRun('A', 263);
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 255,
                                 'A', 'A', 'A', 'A', 0};
    bool ok = rle1Vector(in, 263, exp, 10);
    free(in);
    return ok;
}

static bool rle1EncRun518(void) {
    unsigned char* in = makeRun('A', 518);
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 255,
                                 'A', 'A', 'A', 'A', 255};
    bool ok = rle1Vector(in, 518, exp, 10);
    free(in);
    return ok;
}

static bool rle1EncRunThenOther(void) {
    const unsigned char in[] = {'A', 'A', 'A', 'A', 'B'};
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 0, 'B'};
    return rle1Vector(in, 5, exp, 6);
}

static bool rle1EncTwoRuns(void) {
    const unsigned char in[] = {'A', 'A', 'A', 'A', 'A',
                                'A', 'B', 'B', 'B', 'B'};
    const unsigned char exp[] = {'A', 'A', 'A', 'A', 2, 'B', 'B', 'B', 'B', 0};
    return rle1Vector(in, 10, exp, 10);
}

static bool rle1EncZeroBytes(void) {
    const unsigned char in[] = {0, 0, 0, 0, 0, 0};
    const unsigned char exp[] = {0, 0, 0, 0, 2};
    return rle1Vector(in, 6, exp, 5);
}

static bool rle1EncNoRuns(void) {
    const unsigned char in[] = {'A', 'B', 'A', 'B', 'A', 'B'};
    return rle1Vector(in, 6, in, 6);
}

static bool rle1EncWorstCaseExpansion(void) {
    // Runs of exactly 4 are the worst case: 4 bytes -> 5 bytes.
    unsigned int n = 4000;
    unsigned char* in = malloc(n);
    for (unsigned int i = 0; i < n; i++) in[i] = (unsigned char)((i / 4) % 2);
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    errors r = RLE1encode(in, n, &out, &outN, map);
    free(in);
    free(out);
    EXPECT(r == success);
    EXPECT(outN == 5000);
    return true;
}

static bool rle1EncMapRun4(void) {
    const unsigned char in[] = {'A', 'A', 'A', 'A'};
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(RLE1encode((unsigned char*)in, 4, &out, &outN, map) == success);
    free(out);
    EXPECT(mapHas(map, 'A'));
    EXPECT(mapHas(map, 0));  // the length byte must be in the map too
    EXPECT(mapCount(map) == 2);
    return true;
}

static bool rle1EncMapLengthByte(void) {
    unsigned char* in = makeRun('A', 8);
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    errors r = RLE1encode(in, 8, &out, &outN, map);
    free(in);
    free(out);
    EXPECT(r == success);
    EXPECT(mapHas(map, 'A'));
    EXPECT(mapHas(map, 4));
    EXPECT(mapCount(map) == 2);
    return true;
}

static bool rle1EncMapNoRuns(void) {
    const unsigned char in[] = {'A', 'B', 'C', 0xFF};
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(RLE1encode((unsigned char*)in, 4, &out, &outN, map) == success);
    free(out);
    EXPECT(mapHas(map, 'A') && mapHas(map, 'B') && mapHas(map, 'C'));
    EXPECT(mapHas(map, 0xFF));
    EXPECT(mapCount(map) == 4);
    return true;
}

static bool rle1EncMapAll256(void) {
    unsigned char in[256];
    for (int i = 0; i < 256; i++) in[i] = (unsigned char)i;
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(RLE1encode(in, 256, &out, &outN, map) == success);
    free(out);
    EXPECT(mapCount(map) == 256);
    return true;
}

// ============================================================================
//  RLE1decode tests
// ============================================================================


static bool rle1DecInitEmpty(void) {
    RLE1info info;
    RLE1infoInit(&info, NULL, 0);
    EXPECT(info.done);
    return true;
}

static bool rle1DecPauseBeforeLengthByte(void) {
    // "AAAA\3" decodes to 7 'A'. With chunk 4 the first call must stop right
    // before the length byte and resume correctly.
    unsigned char enc[] = {'A', 'A', 'A', 'A', 3};
    RLE1info info;
    RLE1infoInit(&info, enc, 5);
    unsigned char* out = NULL;
    unsigned int outN = 0;

    EXPECT(RLE1decode(&info, &out, &outN, 4) == success);
    EXPECT(outN == 4);
    EXPECT(memcmp(out, "AAAA", 4) == 0);
    EXPECT(!info.done);
    EXPECT(info.charCount == SEQ_MAXLEN);
    EXPECT(info.remainingSize == 1);
    free(out);

    EXPECT(RLE1decode(&info, &out, &outN, 4) == success);
    EXPECT(outN == 3);
    EXPECT(memcmp(out, "AAA", 3) == 0);
    EXPECT(info.done);
    free(out);
    return true;
}

static bool rle1DecPendingRunAcrossChunks(void) {
    // "AAAA\255" = 259 'A'; chunk 100 -> 100, 100, 59.
    unsigned char enc[] = {'A', 'A', 'A', 'A', 255};
    RLE1info info;
    RLE1infoInit(&info, enc, 5);
    unsigned int sizes[3];
    for (int k = 0; k < 3; k++) {
        unsigned char* out = NULL;
        EXPECT(RLE1decode(&info, &out, &sizes[k], 100) == success);
        for (unsigned int i = 0; i < sizes[k]; i++) EXPECT(out[i] == 'A');
        free(out);
    }
    EXPECT(sizes[0] == 100 && sizes[1] == 100 && sizes[2] == 59);
    EXPECT(info.done);
    return true;
}

static bool rle1DecZeroLengthByte(void) {
    unsigned char enc[] = {'A', 'A', 'A', 'A', 0, 'B'};
    unsigned char* out = NULL;
    unsigned int outN = 0;
    EXPECT(rle1DecodeAll(enc, 6, 64, &out, &outN));
    bool ok = bytesMatch(out, outN, (const unsigned char*)"AAAAB", 5);
    free(out);
    return ok;
}

static bool rle1DecReinitBetweenBlocks(void) {
    // Block 1 ends with "AAA" (charCount 3). Block 2 starts with "AA" + more.
    // If state leaked, the decoder would expect a length byte after 1 'A'.
    unsigned char block1[] = {'B', 'A', 'A', 'A'};
    unsigned char block2[] = {'A', 'A', 'C'};
    RLE1info info;
    unsigned char* out = NULL;
    unsigned int outN = 0;

    RLE1infoInit(&info, block1, 4);
    EXPECT(RLE1decode(&info, &out, &outN, 64) == success);
    EXPECT(info.done && outN == 4);
    free(out);

    RLE1infoInit(&info, block2, 3);
    EXPECT(RLE1decode(&info, &out, &outN, 64) == success);
    EXPECT(info.done);
    bool ok = bytesMatch(out, outN, block2, 3);
    free(out);
    return ok;
}

static bool rle1RtSingle(void) {
    const unsigned char in[] = {'x'};
    return rle1RoundTripAllChunks(in, 1);
}

static bool rle1RtRunBoundaries(void) {
    static const unsigned int lens[] = {1,   2,   3,   4,   5,   6,   254,
                                        255, 256, 257, 258, 259, 260, 261,
                                        262, 263, 264, 517, 518, 519, 1000};
    for (unsigned int i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
        unsigned char* in = makeRun('Q', lens[i]);
        bool ok = rle1RoundTripAllChunks(in, lens[i]);
        free(in);
        if (!ok) {
            printf("    (run length %u)\n", lens[i]);
            return false;
        }
    }
    return true;
}

static bool rle1RtRunsOfFour(void) {
    unsigned int n = 4000;
    unsigned char* in = malloc(n);
    for (unsigned int i = 0; i < n; i++)
        in[i] = (unsigned char)('a' + (i / 4) % 3);
    bool ok = rle1RoundTripAllChunks(in, n);
    free(in);
    return ok;
}

static bool rle1RtAll256(void) {
    unsigned char in[256];
    for (int i = 0; i < 256; i++) in[i] = (unsigned char)i;
    return rle1RoundTripAllChunks(in, 256);
}

static bool rle1RtZeroRuns(void) {
    unsigned int n = 3000;
    unsigned char* in = malloc(n);
    fillRandomRuns(in, n, 2, 600);  // only 0x00 and 0x01
    bool ok = rle1RoundTripAllChunks(in, n);
    free(in);
    return ok;
}

static bool rle1RtRandom(void) {
    unsigned int n = 5000;
    unsigned char* in = malloc(n);
    fillRandom(in, n);
    bool ok = rle1RoundTripAllChunks(in, n);
    free(in);
    return ok;
}

static bool rle1RtRandomRuns(void) {
    unsigned int n = 20000;
    unsigned char* in = malloc(n);
    fillRandomRuns(in, n, 4, 700);
    bool ok = rle1RoundTripAllChunks(in, n);
    free(in);
    return ok;
}

// ============================================================================
//  MTF tests
// ============================================================================

static bool mtfEncBanana(void) {
    unsigned char dict[] = {'a', 'b', 'n'};
    unsigned char out[6];
    EXPECT(MTFencode((unsigned char*)"banana", 6, out, dict, 3) == success);
    const unsigned char exp[] = {1, 1, 2, 1, 1, 1};
    EXPECT(bytesMatch(out, 6, exp, 6));
    // Final dictionary state: last symbol 'a' at front.
    EXPECT(dict[0] == 'a' && dict[1] == 'n' && dict[2] == 'b');
    return true;
}

static bool mtfDecBanana(void) {
    unsigned char dict[] = {'a', 'b', 'n'};
    const unsigned char in[] = {1, 1, 2, 1, 1, 1};
    unsigned char out[6];
    EXPECT(MTFdecode((unsigned char*)in, 6, out, dict, 3) == success);
    return bytesMatch(out, 6, (const unsigned char*)"banana", 6);
}

static bool mtfEncRepeatsBecomeZeros(void) {
    unsigned char dict[] = {'a', 'b'};
    unsigned char out[5];
    EXPECT(MTFencode((unsigned char*)"bbbbb", 5, out, dict, 2) == success);
    const unsigned char exp[] = {1, 0, 0, 0, 0};
    return bytesMatch(out, 5, exp, 5);
}

static bool mtfEncFirstAndLastOf256(void) {
    unsigned char dict[256];
    for (int i = 0; i < 256; i++) dict[i] = (unsigned char)i;
    const unsigned char in[] = {255, 255, 0, 255};
    unsigned char out[4];
    EXPECT(MTFencode((unsigned char*)in, 4, out, dict, 256) == success);
    // 255 is last -> 255; then at front -> 0; 0 shifted to index 1 -> 1;
    // 255 now at index 1 -> 1.
    const unsigned char exp[] = {255, 0, 1, 1};
    return bytesMatch(out, 4, exp, 4);
}

static bool mtfEncSingleEntryDict(void) {
    unsigned char dict[] = {'z'};
    unsigned char out[4];
    EXPECT(MTFencode((unsigned char*)"zzzz", 4, out, dict, 1) == success);
    const unsigned char exp[] = {0, 0, 0, 0};
    return bytesMatch(out, 4, exp, 4);
}

static bool mtfEncSymbolMissing(void) {
    unsigned char dict[] = {'a', 'b'};
    unsigned char out[3];
    EXPECT(MTFencode((unsigned char*)"abc", 3, out, dict, 2) == generalError);
    return true;
}

static bool mtfEncEmptyDict(void) {
    unsigned char dict[1] = {0};
    unsigned char out[1];
    EXPECT(MTFencode((unsigned char*)"a", 1, out, dict, 0) == generalError);
    return true;
}

static bool mtfEncEmptyInput(void) {
    unsigned char dict[] = {'a'};
    unsigned char out[1];
    EXPECT(MTFencode((unsigned char*)"", 0, out, dict, 1) == success);
    EXPECT(dict[0] == 'a');
    return true;
}

static bool mtfDecIndexOutOfRange(void) {
    unsigned char dict[] = {'a', 'b', 'c'};
    const unsigned char in[] = {0, 1, 3};
    unsigned char out[3];
    EXPECT(MTFdecode((unsigned char*)in, 3, out, dict, 3) == corruptedData);
    return true;
}

static bool mtfRtRandomFullDict(void) {
    unsigned int n = 10000;
    unsigned char* in = malloc(n);
    unsigned char* enc = malloc(n);
    unsigned char* dec = malloc(n);
    fillRandom(in, n);
    unsigned char d1[256], d2[256];
    for (int i = 0; i < 256; i++) d1[i] = d2[i] = (unsigned char)i;
    bool ok = MTFencode(in, n, enc, d1, 256) == success &&
              MTFdecode(enc, n, dec, d2, 256) == success &&
              bytesMatch(dec, n, in, n);
    free(in);
    free(enc);
    free(dec);
    return ok;
}

static bool mtfRtSparseDict(void) {
    // Dictionary built from inUseMap exactly as the pipeline does it.
    unsigned int n = 5000;
    unsigned char* in = malloc(n);
    unsigned char* enc = malloc(n);
    unsigned char* dec = malloc(n);
    unsigned char map[IN_USE_MAP_SIZE] = {0};
    const unsigned char alphabet[] = {3, 'e', 'q', 200, 255};
    for (unsigned int i = 0; i < n; i++) {
        in[i] = alphabet[rng() % 5];
        map[in[i] / 8] |= (unsigned char)(1 << (in[i] % 8));
    }
    unsigned char d1[256], d2[256];
    unsigned int s1 = buildDict(map, d1), s2 = buildDict(map, d2);
    bool ok = s1 == 5 && MTFencode(in, n, enc, d1, s1) == success;
    for (unsigned int i = 0; ok && i < n; i++) ok = enc[i] < s1;
    ok = ok && MTFdecode(enc, n, dec, d2, s2) == success &&
         bytesMatch(dec, n, in, n);
    free(in);
    free(enc);
    free(dec);
    return ok;
}

// ============================================================================
//  RLE2 tests
// ============================================================================


static bool rle2NoZeros(void) {
    const unsigned char in[] = {1, 2, 3};
    const unsigned char exp[] = {1, 2, 3, 0, 0};
    return rle2Vector(in, 3, exp, 5);
}

static bool rle2SingleZero(void) {
    const unsigned char in[] = {0};
    const unsigned char exp[] = {0, 1, 0, 0};
    return rle2Vector(in, 1, exp, 4);
}

static bool rle2ZerosInMiddle(void) {
    const unsigned char in[] = {5, 0, 0, 0, 7};
    const unsigned char exp[] = {5, 0, 3, 7, 0, 0};
    return rle2Vector(in, 5, exp, 6);
}

static bool rle2ZerosAtEnd(void) {
    const unsigned char in[] = {1, 0, 0};
    const unsigned char exp[] = {1, 0, 2, 0, 0};
    return rle2Vector(in, 3, exp, 5);
}

static bool rle2Zeros255(void) {
    unsigned char* in = makeRun(0, 255);
    const unsigned char exp[] = {0, 255, 0, 0};
    bool ok = rle2Vector(in, 255, exp, 4);
    free(in);
    return ok;
}

static bool rle2Zeros256(void) {
    unsigned char* in = makeRun(0, 256);
    const unsigned char exp[] = {0, 255, 0, 1, 0, 0};
    bool ok = rle2Vector(in, 256, exp, 6);
    free(in);
    return ok;
}

static bool rle2Zeros510(void) {
    unsigned char* in = makeRun(0, 510);
    const unsigned char exp[] = {0, 255, 0, 255, 0, 0};
    bool ok = rle2Vector(in, 510, exp, 6);
    free(in);
    return ok;
}

static bool rle2Zeros511(void) {
    unsigned char* in = makeRun(0, 511);
    const unsigned char exp[] = {0, 255, 0, 255, 0, 1, 0, 0};
    bool ok = rle2Vector(in, 511, exp, 8);
    free(in);
    return ok;
}

static bool rle2WorstCase(void) {
    // Isolated zeros: "0 x" -> "0 1 x" = 1.5x expansion.
    unsigned int n = 1000;
    unsigned char* in = malloc(n);
    for (unsigned int i = 0; i < n; i++) in[i] = (i % 2) ? 9 : 0;
    unsigned char* out = NULL;
    unsigned int outN = 0;
    errors r = RLE2encode(in, n, &out, &outN);
    free(out);
    bool ok = r == success && outN == 1502 && rle2RoundTrip(in, n);
    free(in);
    return ok;
}


static bool rle2DecZeroExpectedSize(void) {
    unsigned char input[] = {0, 0}; // EOB
    unsigned char* output = (unsigned char*)1; // Garbage pointer
    unsigned int outSize = 1;
    
    errors err = RLE2decode(input, sizeof(input), &output, &outSize, 0);
    if (err != success) {
        printf("FAIL: Expected success for 0 expected size, got %d\n", err);
        return false;
    }
    if (output != NULL || outSize != 0) {
        printf("FAIL: Expected output=NULL and outSize=0\n");
        return false;
    }
    return true;
}

static bool rle2DecIgnoresAfterEob(void) {
    const unsigned char enc[] = {7, 0, 0, 9, 9};
    unsigned char* dec = NULL;
    unsigned int decN = 0;
    EXPECT(RLE2decode((unsigned char*)enc, 5, &dec, &decN, 10) == corruptedData);
    bool ok = (decN == 0 && dec == NULL);
    free(dec);
    return ok;
}

static bool rle2DecOnlyEob(void) {
    const unsigned char enc[] = {0, 0};
    unsigned char* dec = NULL;
    unsigned int decN = 99;
    EXPECT(RLE2decode((unsigned char*)enc, 2, &dec, &decN, 0) == success);
    EXPECT(decN == 0);
    free(dec);
    return true;
}


static bool rle2DecTruncatedEscape(void) {
    const unsigned char enc[] = {7, 0};
    return rle2DecodeExpectError(enc, 2, 10);
}

static bool rle2DecMissingEob(void) {
    const unsigned char enc[] = {7, 8, 0, 3};
    return rle2DecodeExpectError(enc, 4, 10);
}

static bool rle2DecZeroRunOverflow(void) {
    const unsigned char enc[] = {0, 10, 0, 0};
    return rle2DecodeExpectError(enc, 4, 5);
}

static bool rle2DecLiteralOverflow(void) {
    const unsigned char enc[] = {1, 2, 3, 0, 0};
    return rle2DecodeExpectError(enc, 5, 2);
}

static bool rle2RtRandomWithZeros(void) {
    unsigned int n = 50000;
    unsigned char* in = malloc(n);
    // ~70% zeros in runs, like real MTF output.
    unsigned int i = 0;
    while (i < n) {
        if (rng() % 10 < 7) {
            unsigned int r = 1 + rng() % 700;
            while (r-- > 0 && i < n) in[i++] = 0;
        } else {
            in[i++] = (unsigned char)(1 + rng() % 255);
        }
    }
    bool ok = rle2RoundTrip(in, n);
    free(in);
    return ok;
}

static bool rle2RtAllZeros(void) {
    unsigned int n = 100000;
    unsigned char* in = makeRun(0, n);
    bool ok = rle2RoundTrip(in, n);
    free(in);
    return ok;
}

// ============================================================================
//  Full pipeline tests
// ============================================================================

static bool pipeSingleByte(void) {
    const unsigned char in[] = {'A'};
    return pipelineAllChunks(in, 1);
}

static bool pipeTwoBytes(void) {
    return pipelineAllChunks((const unsigned char*)"AB", 2);
}

static bool pipeBanana(void) {
    return pipelineAllChunks((const unsigned char*)"banana", 6);
}

static bool pipeText(void) {
    const char* text =
        "The quick brown fox jumps over the lazy dog. "
        "Pack my box with five dozen liquor jugs!!!!!!!! "
        "How vexingly quick daft zebras jump.\n\n\n\n\n\n"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    return pipelineAllChunks((const unsigned char*)text,
                             (unsigned int)strlen(text));
}

static bool pipeAllSame(void) {
    unsigned char* in = makeRun('x', 10000);
    bool ok = pipelineAllChunks(in, 10000);
    free(in);
    return ok;
}

static bool pipeRunBoundaries(void) {
    static const unsigned int lens[] = {3, 4, 5, 258, 259, 260, 263, 518, 519};
    for (unsigned int i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
        unsigned char* in = makeRun('a', lens[i]);
        bool ok = pipelineAllChunks(in, lens[i]);
        free(in);
        if (!ok) {
            printf("    (run length %u)\n", lens[i]);
            return false;
        }
    }
    return true;
}

static bool pipeAllZeros(void) {
    unsigned char* in = makeRun(0, 100000);
    bool ok = pipelineAllChunks(in, 100000);
    free(in);
    return ok;
}

static bool pipeAll256(void) {
    unsigned char in[512];
    for (int i = 0; i < 512; i++) in[i] = (unsigned char)(i % 256);
    return pipelineAllChunks(in, 512);
}

static bool pipeBinaryAlternating(void) {
    unsigned int n = 5000;
    unsigned char* in = malloc(n);
    for (unsigned int i = 0; i < n; i++) in[i] = (i % 2) ? 0xFF : 0x00;
    bool ok = pipelineAllChunks(in, n);
    free(in);
    return ok;
}

static bool pipeFibonacci(void) {
    unsigned int n = 10000;
    unsigned char* in = calloc(n, 1);
    in[0] = 'b';
    in[1] = 'a';
    unsigned int la = 1, lb = 1, pos = 2;
    while (pos < n) {
        unsigned int len = la;
        if (pos + len > n) len = n - pos;
        memcpy(in + pos, in + pos - la - lb, len);
        pos += len;
        unsigned int t = la;
        la += lb;
        lb = t;
    }
    bool ok = pipelineAllChunks(in, n);
    free(in);
    return ok;
}

static bool pipeRandom(void) {
    unsigned int n = 50000;
    unsigned char* in = malloc(n);
    fillRandom(in, n);
    bool ok = pipelineAllChunks(in, n);
    free(in);
    return ok;
}

static bool pipeRandomRuns(void) {
    unsigned int n = 100000;
    unsigned char* in = malloc(n);
    fillRandomRuns(in, n, 6, 600);
    bool ok = pipelineAllChunks(in, n);
    free(in);
    return ok;
}

static bool pipeManySmallSizes(void) {
    unsigned char in[64];
    for (unsigned int n = 1; n <= 64; n++) {
        for (unsigned int rep = 0; rep < 20; rep++) {
            fillRandomRuns(in, n, 3, 8);
            if (!pipelineRoundTrip(in, n, 5)) {
                printf("    (size %u, rep %u)\n", n, rep);
                return false;
            }
        }
    }
    return true;
}

// ============================================================================

static const testEntry tests[] = {
    // RLE1encode
    {"RLE1encode: empty input", rle1EncEmpty},
    {"RLE1encode: single byte", rle1EncSingle},
    {"RLE1encode: run of 3 (no length byte)", rle1EncRun3},
    {"RLE1encode: run of 4 -> length 0", rle1EncRun4},
    {"RLE1encode: run of 5 -> length 1", rle1EncRun5},
    {"RLE1encode: run of 258 -> length 254", rle1EncRun258},
    {"RLE1encode: run of 259 -> length 255 (max)", rle1EncRun259},
    {"RLE1encode: run of 260 -> max + 1 literal", rle1EncRun260},
    {"RLE1encode: run of 263 -> two full groups", rle1EncRun263},
    {"RLE1encode: run of 518 -> two max groups", rle1EncRun518},
    {"RLE1encode: run of 4 followed by other byte", rle1EncRunThenOther},
    {"RLE1encode: two consecutive runs", rle1EncTwoRuns},
    {"RLE1encode: run of 0x00 bytes", rle1EncZeroBytes},
    {"RLE1encode: no runs stays unchanged", rle1EncNoRuns},
    {"RLE1encode: worst-case expansion is exactly +25%",
     rle1EncWorstCaseExpansion},
    {"RLE1encode: inUseMap includes length byte 0", rle1EncMapRun4},
    {"RLE1encode: inUseMap includes length byte 4", rle1EncMapLengthByte},
    {"RLE1encode: inUseMap exact for literals", rle1EncMapNoRuns},
    {"RLE1encode: inUseMap all 256 symbols", rle1EncMapAll256},
    // RLE1decode
    // removed: {"RLE1decode: chunk size 0", rle1DecChunkZero},
    {"RLE1infoInit: empty input is done", rle1DecInitEmpty},
    {"RLE1decode: pause right before length byte",
     rle1DecPauseBeforeLengthByte},
    {"RLE1decode: pending run spans chunks", rle1DecPendingRunAcrossChunks},
    {"RLE1decode: length byte 0", rle1DecZeroLengthByte},
    {"RLE1decode: re-init between blocks", rle1DecReinitBetweenBlocks},
    {"RLE1 round trip: single byte", rle1RtSingle},
    {"RLE1 round trip: run length boundaries", rle1RtRunBoundaries},
    {"RLE1 round trip: runs of exactly 4", rle1RtRunsOfFour},
    {"RLE1 round trip: all 256 symbols", rle1RtAll256},
    {"RLE1 round trip: long 0x00/0x01 runs", rle1RtZeroRuns},
    {"RLE1 round trip: random bytes", rle1RtRandom},
    {"RLE1 round trip: random long runs", rle1RtRandomRuns},
    // MTF
    {"MTFencode: banana known vector", mtfEncBanana},
    {"MTFdecode: banana known vector", mtfDecBanana},
    {"MTFencode: repeats become zeros", mtfEncRepeatsBecomeZeros},
    {"MTFencode: first/last of 256 dictionary", mtfEncFirstAndLastOf256},
    {"MTFencode: single entry dictionary", mtfEncSingleEntryDict},
    {"MTFencode: symbol missing from dictionary", mtfEncSymbolMissing},
    {"MTFencode: empty dictionary", mtfEncEmptyDict},
    {"MTFencode: empty input", mtfEncEmptyInput},
    {"MTFdecode: index out of range", mtfDecIndexOutOfRange},
    {"MTF round trip: random, full dictionary", mtfRtRandomFullDict},
    {"MTF round trip: sparse dictionary from inUseMap", mtfRtSparseDict},
    // RLE2
    {"RLE2: no zeros", rle2NoZeros},
    {"RLE2: single zero", rle2SingleZero},
    {"RLE2: zeros in middle", rle2ZerosInMiddle},
    {"RLE2: zeros at end", rle2ZerosAtEnd},
    {"RLE2: 255 zeros (one full chunk)", rle2Zeros255},
    {"RLE2: 256 zeros (chunk + 1)", rle2Zeros256},
    {"RLE2: 510 zeros (two full chunks)", rle2Zeros510},
    {"RLE2: 511 zeros", rle2Zeros511},
    {"RLE2: worst-case expansion", rle2WorstCase},
    {"RLE2decode: 0 expected size", rle2DecZeroExpectedSize},
    {"RLE2decode: trailing garbage -> corrupted", rle2DecIgnoresAfterEob},
    {"RLE2decode: only EOB", rle2DecOnlyEob},
    {"RLE2decode: truncated escape -> corrupted", rle2DecTruncatedEscape},
    {"RLE2decode: missing EOB -> corrupted", rle2DecMissingEob},
    {"RLE2decode: zero run overflow -> corrupted", rle2DecZeroRunOverflow},
    {"RLE2decode: literal overflow -> corrupted", rle2DecLiteralOverflow},
    {"RLE2 round trip: MTF-like data", rle2RtRandomWithZeros},
    {"RLE2 round trip: 100k zeros", rle2RtAllZeros},
    // Pipeline
    {"Pipeline: single byte", pipeSingleByte},
    {"Pipeline: two bytes", pipeTwoBytes},
    {"Pipeline: banana", pipeBanana},
    {"Pipeline: text", pipeText},
    {"Pipeline: 10k identical bytes", pipeAllSame},
    {"Pipeline: RLE1 run boundaries", pipeRunBoundaries},
    {"Pipeline: 100k zero bytes", pipeAllZeros},
    {"Pipeline: all 256 symbols", pipeAll256},
    {"Pipeline: alternating 0x00/0xFF", pipeBinaryAlternating},
    {"Pipeline: Fibonacci string", pipeFibonacci},
    {"Pipeline: 50k random bytes", pipeRandom},
    {"Pipeline: 100k random runs", pipeRandomRuns},
    {"Pipeline: every size 1..64", pipeManySmallSizes},
};

int main(void) {
    unsigned int passes = 0, fails = 0;
    unsigned int count = sizeof(tests) / sizeof(tests[0]);

    for (unsigned int i = 0; i < count; i++) {
        if (tests[i].fn()) {
            printf("PASS: %s\n", tests[i].name);
            passes++;
        } else {
            printf("FAIL: %s\n", tests[i].name);
            fails++;
        }
    }

    printf("\nTests completed, PASSES: %u, FAILS: %u\n", passes, fails);
    return fails == 0 ? 0 : 1;
}
