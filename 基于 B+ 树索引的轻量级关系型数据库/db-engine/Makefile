CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c src/buffer_pool.c src/lexer.c
TEST_DISK_SRCS = tests/test_disk_manager.c
TEST_BPT_SRCS = tests/test_bpt.c
TEST_BUFFER_SRCS = tests/test_buffer.c
TEST_LEXER_SRCS = tests/test_lexer.c
OBJS = $(SRCS:.c=.o)
TEST_DISK_OBJS = $(TEST_DISK_SRCS:.c=.o)
TEST_BPT_OBJS = $(TEST_BPT_SRCS:.c=.o)
TEST_BUFFER_OBJS = $(TEST_BUFFER_SRCS:.c=.o)
TEST_LEXER_OBJS = $(TEST_LEXER_SRCS:.c=.o)

TARGET = libdbengine.a
TEST_DISK_TARGET = test_disk_manager
TEST_BPT_TARGET = test_bpt
TEST_BUFFER_TARGET = test_buffer
TEST_LEXER_TARGET = test_lexer

all: $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(TEST_DISK_TARGET): $(TEST_DISK_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BPT_TARGET): $(TEST_BPT_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BUFFER_TARGET): $(TEST_BUFFER_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_LEXER_TARGET): $(TEST_LEXER_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-del src\disk_manager.o src\bplus_tree.o src\buffer_pool.o src\lexer.o tests\test_disk_manager.o tests\test_bpt.o tests\test_buffer.o tests\test_lexer.o libdbengine.a test_disk_manager.exe test_bpt.exe test_buffer.exe test_lexer.exe *.db 2>NUL

test: $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET)
	@echo "=== Disk Manager Tests ==="
	./$(TEST_DISK_TARGET)
	@echo ""
	@echo "=== B+ Tree Tests ==="
	./$(TEST_BPT_TARGET)
	@echo ""
	@echo "=== Buffer Pool Tests ==="
	./$(TEST_BUFFER_TARGET)
	@echo ""
	@echo "=== Lexer Tests ==="
	./$(TEST_LEXER_TARGET)

test-bpt: $(TEST_BPT_TARGET)
	./$(TEST_BPT_TARGET)

test-disk: $(TEST_DISK_TARGET)
	./$(TEST_DISK_TARGET)

test-buffer: $(TEST_BUFFER_TARGET)
	./$(TEST_BUFFER_TARGET)

test-lexer: $(TEST_LEXER_TARGET)
	./$(TEST_LEXER_TARGET)

.PHONY: all clean test test-bpt test-disk test-buffer test-lexer
