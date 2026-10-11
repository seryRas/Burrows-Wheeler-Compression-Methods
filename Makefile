CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -g

BZIP2_TARGET = bzip2
TEST_TARGET = bwt-test
BZIP2_TEST_TARGET = bzip2-test

BZIP2_SRCS = bzip2.c BWT.c rle.c mtf.c bitOperators.c huffmanEncoding.c
TEST_SRCS = bwt-test.c
BZIP2_TEST_SRCS = bzip2-test.c

BZIP2_OBJS = $(BZIP2_SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o) BWT.o rle.o mtf.o bitOperators.o
BZIP2_TEST_OBJS = $(BZIP2_TEST_SRCS:.c=.o) bzip2-nomain.o BWT.o rle.o mtf.o bitOperators.o huffmanEncoding.o

ALL_OBJS = bzip2.o BWT.o bwt-test.o bzip2-test.o bzip2-nomain.o rle.o mtf.o bitOperators.o huffmanEncoding.o
HEADERS = BWT.h bzip2.h rle.h mtf.h errors.h huffmanEncoding.h bitOperators.h

.PHONY: all clean test test-bzip2

all: $(BZIP2_TARGET)

$(BZIP2_TARGET): $(BZIP2_OBJS)
	$(CC) $(CFLAGS) -o $(BZIP2_TARGET) $(BZIP2_OBJS)
	rm -f $(BZIP2_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

bzip2-nomain.o: bzip2.c $(HEADERS)
	$(CC) $(CFLAGS) -DBZIP2_TEST -c bzip2.c -o $@

clean:
	rm -f $(ALL_OBJS) $(BZIP2_TARGET) $(TEST_TARGET) $(BZIP2_TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)
	$(MAKE) clean

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS)
	rm -f $(TEST_OBJS)

test-bzip2: $(BZIP2_TEST_TARGET)
	./$(BZIP2_TEST_TARGET)
	$(MAKE) clean

$(BZIP2_TEST_TARGET): $(BZIP2_TEST_OBJS)
	$(CC) $(CFLAGS) -o $(BZIP2_TEST_TARGET) $(BZIP2_TEST_OBJS)
	rm -f $(BZIP2_TEST_OBJS)
