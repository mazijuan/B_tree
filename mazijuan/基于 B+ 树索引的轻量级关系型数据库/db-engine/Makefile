CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c src/buffer_pool.c src/lexer.c src/parser.c src/executor.c src/database.c
TEST_DISK_SRCS = tests/test_disk_manager.c
TEST_BPT_SRCS = tests/test_bpt.c
TEST_BUFFER_SRCS = tests/test_buffer_pool.c
TEST_LEXER_SRCS = tests/test_lexer.c
TEST_SQL_SRCS = tests/test_sql.c
OBJS = $(SRCS:.c=.o)
TEST_DISK_OBJS = $(TEST_DISK_SRCS:.c=.o)
TEST_BPT_OBJS = $(TEST_BPT_SRCS:.c=.o)
TEST_BUFFER_OBJS = $(TEST_BUFFER_SRCS:.c=.o)
TEST_LEXER_OBJS = $(TEST_LEXER_SRCS:.c=.o)
TEST_SQL_OBJS = $(TEST_SQL_SRCS:.c=.o)

TARGET = libdbengine.a
TEST_DISK_TARGET = test_disk_manager
TEST_BPT_TARGET = test_bpt
TEST_BUFFER_TARGET = test_buffer_pool
TEST_LEXER_TARGET = test_lexer
TEST_SQL_TARGET = test_sql

all: $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET) $(TEST_SQL_TARGET)

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

$(TEST_SQL_TARGET): $(TEST_SQL_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TEST_DISK_OBJS) $(TEST_BPT_OBJS) $(TEST_BUFFER_OBJS) $(TEST_LEXER_OBJS) $(TEST_SQL_OBJS) $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET) $(TEST_SQL_TARGET) *.db

test: $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET) $(TEST_SQL_TARGET)
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
	@echo ""
	@echo "=== SQL Tests ==="
	./$(TEST_SQL_TARGET)

test-bpt: $(TEST_BPT_TARGET)
	./$(TEST_BPT_TARGET)

test-disk: $(TEST_DISK_TARGET)
	./$(TEST_DISK_TARGET)

test-buffer: $(TEST_BUFFER_TARGET)
	./$(TEST_BUFFER_TARGET)

test-lexer: $(TEST_LEXER_TARGET)
	./$(TEST_LEXER_TARGET)

test-sql: $(TEST_SQL_TARGET)
	./$(TEST_SQL_TARGET)

.PHONY: all clean test test-bpt test-disk test-buffer test-lexer test-sql
