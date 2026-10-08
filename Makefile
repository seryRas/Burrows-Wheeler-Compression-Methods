CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -g

MAIN_TARGET = main
BZIP2_TARGET = bzip2
TEST_TARGET = bwt-test
BZIP2_TEST_TARGET = bzip2-test

MAIN_SRCS = main.c BWT.c rle.c mtf.c bitOperators.c
BZIP2_SRCS = bzip2.c BWT.c rle.c mtf.c bitOperators.c
TEST_SRCS = bwt-test.c
BZIP2_TEST_SRCS = bzip2-test.c

MAIN_OBJS = $(MAIN_SRCS:.c=.o)
BZIP2_OBJS = $(BZIP2_SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o) BWT.o rle.o mtf.o bitOperators.o
BZIP2_TEST_OBJS = $(BZIP2_TEST_SRCS:.c=.o) bzip2-nomain.o BWT.o rle.o mtf.o bitOperators.o

ALL_OBJS = main.o bzip2.o BWT.o bwt-test.o bzip2-test.o bzip2-nomain.o rle.o mtf.o bitOperators.o
HEADERS = BWT.h bzip2.h rle.h mtf.h errors.h

.PHONY: all clean test test-bzip2

all: $(MAIN_TARGET)

bzip2: $(BZIP2_TARGET)

$(MAIN_TARGET): $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN_TARGET) $(MAIN_OBJS)

$(BZIP2_TARGET): $(BZIP2_OBJS)
	$(CC) $(CFLAGS) -o $(BZIP2_TARGET) $(BZIP2_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

bzip2-nomain.o: bzip2.c $(HEADERS)
	$(CC) $(CFLAGS) -DBZIP2_TEST -c bzip2.c -o $@

clean:
	rm -f $(ALL_OBJS) $(MAIN_TARGET) $(BZIP2_TARGET) $(TEST_TARGET) $(BZIP2_TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)
	$(MAKE) clean

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS)

test-bzip2: $(BZIP2_TEST_TARGET)
	./$(BZIP2_TEST_TARGET)
	$(MAKE) clean

$(BZIP2_TEST_TARGET): $(BZIP2_TEST_OBJS)
	$(CC) $(CFLAGS) -o $(BZIP2_TEST_TARGET) $(BZIP2_TEST_OBJS)
