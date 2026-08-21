CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -g
TARGET = bwt
RADIX_TARGET = bwt-radix
TEST_TARGET = bwt-test
TEST_RADIX_TARGET = bwt-test-radix
SAIS_SRCS = main.c BWT-SAIS.c
RADIX_SRCS = main.c BWT-radix_sort.c
TEST_SRCS = bwt-test.c
SAIS_OBJS = $(SAIS_SRCS:.c=.o)
RADIX_OBJS = $(RADIX_SRCS:.c=.o)
TEST_SAIS_OBJS = $(TEST_SRCS:.c=.o) BWT-SAIS.o
TEST_RADIX_OBJS = $(TEST_SRCS:.c=.o) BWT-radix_sort.o
ALL_OBJS = main.o BWT-SAIS.o BWT-radix_sort.o bwt-test.o
HEADERS = BWT.h

.PHONY: all clean radix test test-radix

all: $(TARGET)

radix: $(RADIX_TARGET)

$(TARGET): $(SAIS_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SAIS_OBJS)

$(RADIX_TARGET): $(RADIX_OBJS)
	$(CC) $(CFLAGS) -o $(RADIX_TARGET) $(RADIX_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJS) $(TARGET) $(RADIX_TARGET) $(TEST_TARGET) $(TEST_RADIX_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)
	$(MAKE) clean

$(TEST_TARGET): $(TEST_SAIS_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SAIS_OBJS)

test-radix: $(TEST_RADIX_TARGET)
	./$(TEST_RADIX_TARGET)
	$(MAKE) clean

$(TEST_RADIX_TARGET): $(TEST_RADIX_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_RADIX_TARGET) $(TEST_RADIX_OBJS)
