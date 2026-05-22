CC = gcc
PY_CFLAGS = $(shell python3-config --cflags)
PY_LDFLAGS = $(shell python3-config --ldflags --embed)
CFLAGS = -Wall -Wextra -Werror -Iinclude -O3 $(PY_CFLAGS)
LDFLAGS = -lpthread $(PY_LDFLAGS)

SRCS = $(shell find core terminal render ui buffer storage python -name "*.c" 2>/dev/null)
OBJS = $(SRCS:.c=.o)
TARGET = interm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) tests/test_buffer tests/test_buffer.o

test: tests/test_buffer
	./tests/test_buffer

tests/test_buffer: tests/test_buffer.c buffer/buffer.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: all clean test
