#include "huffmanEncoding.h"

#include <stdbool.h>
#include <string.h>

#include "bitOperators.c"
#include "errors.h"

static inline void mapCodes(HuffmanMapping* map, unsigned short* nextCodes) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (map[i].length == 0) continue;
        map[i].code = nextCodes[map[i].length]++;
    }
}

static inline void insertNode(unsigned short* heapArray, unsigned short heapIdx,
                              unsigned short nodeIdx, HuffmanNode* tree) {
    heapArray[heapIdx] = nodeIdx;
    while (heapIdx > 0 &&
           tree[nodeIdx].amount < tree[heapArray[heapIdx - 1]].amount) {
        heapArray[heapIdx] = heapArray[heapIdx - 1];
        heapArray[--heapIdx] = nodeIdx;
    }
}

static inline void combineNodes(unsigned short* heapArray,
                                unsigned short* heapSize, HuffmanNode* tree,
                                unsigned short treeIdx) {
    tree[treeIdx].amount =
        tree[heapArray[0]].amount + tree[heapArray[1]].amount;
    tree[heapArray[0]].parentIdx = treeIdx;
    tree[heapArray[1]].parentIdx = treeIdx;
    heapArray[0] = treeIdx;
    for (int i = 2; i < *heapSize; i++) heapArray[i - 1] = heapArray[i];
    (*heapSize)--;
    int i = 0;
    while (i < (*heapSize) - 1 &&
           tree[heapArray[i]].amount > tree[heapArray[i + 1]].amount) {
        heapArray[i] = heapArray[i + 1];
        heapArray[++i] = treeIdx;
    }
}

static inline void getInitialCodes(unsigned short* codesArray,
                                   unsigned char* depthCounts) {
    unsigned short currentCode = 0;
    for (int i = 1; i <= MAX_DEPTH; i++) {
        codesArray[i] = currentCode;
        currentCode = (currentCode + depthCounts[i]) << 1;
    }
}

static inline unsigned short getDepth(HuffmanNode* tree, unsigned short nodeIdx,
                                      HuffmanMapping* map) {
    unsigned short depth = 0;
    HuffmanNode node = tree[nodeIdx];
    while (node.parentIdx != ROOT_PARENT) {
        depth++;
        node = tree[node.parentIdx];
    }
    map[nodeIdx].length = (unsigned char)depth;
    if (depth > MAX_DEPTH) return 0;
    return depth;
}

errors huffmanEncode(unsigned int* frequencyMap, unsigned char* input,
                     unsigned char** output, unsigned int inputSize,
                     unsigned int* outputSize) {
    if (inputSize == 0) {
        *output = NULL;
        *outputSize = 0;
        return success;
    }

    unsigned int localFreq[ALPHABET_SIZE] = {0};
    if (!frequencyMap) {
        for (unsigned int i = 0; i < inputSize; i++) localFreq[input[i]]++;
        frequencyMap = localFreq;
    }

    unsigned int workingFreq[ALPHABET_SIZE];
    memcpy(workingFreq, frequencyMap, sizeof(workingFreq));

    HuffmanNode tree[MAX_NODE_AMOUNT] = {0};
    unsigned short treeIdx = ALPHABET_SIZE;
    unsigned short heapArr[ALPHABET_SIZE] = {0};
    unsigned short heapSize = 0;
    unsigned char depthCounts[MAX_DEPTH + 1] = {0};
    HuffmanMapping map[ALPHABET_SIZE] = {0};

    bool tooDeep;
    do {
        tooDeep = false;
        memset(tree, 0, sizeof(tree));
        memset(heapArr, 0, sizeof(heapArr));
        heapSize = 0;
        treeIdx = ALPHABET_SIZE;

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (workingFreq[i] > 0) {
                tree[i].amount = workingFreq[i];
                insertNode(heapArr, heapSize++, i, tree);
            }
        }

        if (heapSize == 1) {
            unsigned int outSize =
                (inputSize + 7) / BITS_IN_BYTE + HUFFMAN_HEADER;
            *output = calloc(outSize, sizeof(unsigned char));
            if (!(*output)) return mallocErr;
            (*output)[heapArr[0]] = 1;
            *outputSize = outSize;
            return success;
        }

        while (heapSize > 1) combineNodes(heapArr, &heapSize, tree, treeIdx++);
        tree[heapArr[0]].parentIdx = ROOT_PARENT;

        memset(depthCounts, 0, sizeof(depthCounts));
        memset(map, 0, sizeof(map));

        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (tree[i].amount == 0) continue;
            unsigned short d = getDepth(tree, i, map);
            if (d == 0) {
                tooDeep = true;
                break;
            }
            depthCounts[d]++;
        }

        if (tooDeep) {
            for (int i = 0; i < ALPHABET_SIZE; i++) {
                if (workingFreq[i] > 0) {
                    workingFreq[i] = (workingFreq[i] / 2) + 1;
                }
            }
        }
    } while (tooDeep);

    unsigned short nextCode[MAX_DEPTH + 1] = {0};
    getInitialCodes(nextCode, depthCounts);
    mapCodes(map, nextCode);

    unsigned int payloadBits = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        payloadBits += frequencyMap[i] * map[i].length;

    unsigned int payloadBytes = (payloadBits + 7) / BITS_IN_BYTE;

    *output = malloc(sizeof(unsigned char) * payloadBytes + HUFFMAN_HEADER);
    if (!(*output)) return mallocErr;

    writerData writerInfo = {0};
    for (; writerInfo.outputIdx < ALPHABET_SIZE; writerInfo.outputIdx++) {
        (*output)[writerInfo.outputIdx] = map[writerInfo.outputIdx].length;
    }

    for (unsigned int i = 0; i < inputSize; i++) {
        writeData(map[input[i]].length, map[input[i]].code, *output,
                  &writerInfo);
    }

    if (writerInfo.bitAmount > 0) {
        (*output)[writerInfo.outputIdx++] =
            (unsigned char)(writerInfo.leftoverBits >> 24);
    }

    *outputSize = (unsigned int)writerInfo.outputIdx;
    return success;
}

errors huffmanDecode(unsigned char* input, unsigned int inputSize,
                     unsigned char** output, unsigned int* outputSize,
                     unsigned int expectedOutputSize) {
    if (expectedOutputSize == 0) {
        *output = NULL;
        *outputSize = 0;
        return success;
    }

    if (inputSize < HUFFMAN_HEADER) return corruptedData;

    unsigned char* lengths = input;
    unsigned char depthCounts[MAX_DEPTH + 1] = {0};
    int activeSymbols = 0;
    int singleSymbol = -1;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (lengths[i] > 0) {
            if (lengths[i] > MAX_DEPTH) return corruptedData;
            depthCounts[lengths[i]]++;
            activeSymbols++;
            singleSymbol = i;
        }
    }

    if (activeSymbols == 0) return corruptedData;

    *output = malloc(expectedOutputSize);
    if (!(*output)) return mallocErr;

    if (activeSymbols == 1) {
        memset(*output, (unsigned char)singleSymbol, expectedOutputSize);
        *outputSize = expectedOutputSize;
        return success;
    }

    unsigned short nextCode[MAX_DEPTH + 1] = {0};
    getInitialCodes(nextCode, depthCounts);

    DecodeEntry decodeTable[1 << MAX_DEPTH] = {0};

    for (int sym = 0; sym < ALPHABET_SIZE; sym++) {
        unsigned char len = lengths[sym];
        if (len == 0) continue;
        unsigned short code = nextCode[len]++;
        unsigned int baseIdx = (unsigned int)code << (MAX_DEPTH - len);
        unsigned int count = 1U << (MAX_DEPTH - len);
        for (unsigned int j = 0; j < count; j++) {
            decodeTable[baseIdx + j].symbol = (unsigned char)sym;
            decodeTable[baseIdx + j].length = len;
        }
    }

    readerData reader = {0};
    reader.inputSize = inputSize - HUFFMAN_HEADER;
    unsigned char* bitstream = input + HUFFMAN_HEADER;

    for (unsigned int outIdx = 0; outIdx < expectedOutputSize; outIdx++) {
        unsigned short peeked = readerGetBits(MAX_DEPTH, &reader, bitstream);
        DecodeEntry entry = decodeTable[peeked];
        if (entry.length == 0) {
            free(*output);
            *output = NULL;
            *outputSize = 0;
            return corruptedData;
        }
        (*output)[outIdx] = entry.symbol;
        readerMoveBits(entry.length, &reader);
    }

    *outputSize = expectedOutputSize;
    return success;
}
