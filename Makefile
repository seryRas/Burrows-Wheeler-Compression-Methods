CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = bwt
SRCS = main.c BWT.c
OBJS = $(SRCS:.c=.o)
HEADERS = BWT.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
