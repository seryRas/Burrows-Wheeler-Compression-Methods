CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = bwt
TEST_TARGET = bwt-test
SRCS = main.c BWT.c
OBJS = $(SRCS:.c=.o)
HEADERS = BWT.h

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET)

test:
	@set -e; \
	trap '$(MAKE) clean >/dev/null' EXIT; \
	$(CC) $(CFLAGS) -o $(TEST_TARGET) BWT.c bwt-test.c; \
	./$(TEST_TARGET)
