CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -O2
LDFLAGS = -lpthread

SRCS = $(shell find core platform terminal render ui input buffer syntax commands lsp plugins python ai config storage git -name "*.c" 2>/dev/null)
OBJS = $(SRCS:.c=.o)
TARGET = interm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BINS)

test: tests/test_buffer
	./tests/test_buffer

tests/test_buffer: tests/test_buffer.c buffer/buffer.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: all clean test
