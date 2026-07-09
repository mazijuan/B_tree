CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c src/buffer_pool.c src/lexer.c src/parser.c src/executor.c src/database.c src/wal.c src/cli.c
TEST_DISK_SRCS = tests/test_disk_manager.c
TEST_BPT_SRCS = tests/test_bpt.c
TEST_BUFFER_SRCS = tests/test_buffer.c
TEST_LEXER_SRCS = tests/test_lexer.c
TEST_PARSER_SRCS = tests/test_parser.c
TEST_EXECUTOR_SRCS = tests/test_executor.c
TEST_DATABASE_SRCS = tests/test_database.c
TEST_STRESS_SRCS = tests/stress_test.c
OBJS = $(SRCS:.c=.o)
TEST_DISK_OBJS = $(TEST_DISK_SRCS:.c=.o)
TEST_BPT_OBJS = $(TEST_BPT_SRCS:.c=.o)
TEST_BUFFER_OBJS = $(TEST_BUFFER_SRCS:.c=.o)
TEST_LEXER_OBJS = $(TEST_LEXER_SRCS:.c=.o)
TEST_PARSER_OBJS = $(TEST_PARSER_SRCS:.c=.o)
TEST_EXECUTOR_OBJS = $(TEST_EXECUTOR_SRCS:.c=.o)
TEST_DATABASE_OBJS = $(TEST_DATABASE_SRCS:.c=.o)
TEST_STRESS_OBJS = $(TEST_STRESS_SRCS:.c=.o)

TARGET = libdbengine.a
TEST_DISK_TARGET = test_disk_manager
TEST_BPT_TARGET = test_bpt
TEST_BUFFER_TARGET = test_buffer
TEST_LEXER_TARGET = test_lexer
TEST_PARSER_TARGET = test_parser
TEST_EXECUTOR_TARGET = test_executor
TEST_DATABASE_TARGET = test_database
TEST_STRESS_TARGET = stress_test
CLI_TARGET = dbcli

all: $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET) $(TEST_PARSER_TARGET) $(TEST_EXECUTOR_TARGET) $(TEST_DATABASE_TARGET) $(CLI_TARGET)

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

$(TEST_PARSER_TARGET): $(TEST_PARSER_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_EXECUTOR_TARGET): $(TEST_EXECUTOR_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_DATABASE_TARGET): $(TEST_DATABASE_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_STRESS_TARGET): $(TEST_STRESS_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLI_TARGET): src/cli.o $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-del src\disk_manager.o src\bplus_tree.o src\buffer_pool.o src\lexer.o src\parser.o src\executor.o src\database.o src\wal.o src\cli.o tests\test_disk_manager.o tests\test_bpt.o tests\test_buffer.o tests\test_lexer.o tests\test_parser.o tests\test_executor.o tests\test_database.o tests\stress_test.o libdbengine.a test_disk_manager.exe test_bpt.exe test_buffer.exe test_lexer.exe test_parser.exe test_executor.exe test_database.exe stress_test.exe dbcli.exe *.db *.wal 2>NUL

test: $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) $(TEST_BUFFER_TARGET) $(TEST_LEXER_TARGET) $(TEST_PARSER_TARGET) $(TEST_EXECUTOR_TARGET) $(TEST_DATABASE_TARGET)
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
	@echo "=== Parser Tests ==="
	./$(TEST_PARSER_TARGET)
	@echo ""
	@echo "=== Executor Tests ==="
	./$(TEST_EXECUTOR_TARGET)
	@echo ""
	@echo "=== Database API Tests ==="
	./$(TEST_DATABASE_TARGET)

test-bpt: $(TEST_BPT_TARGET)
	./$(TEST_BPT_TARGET)

test-disk: $(TEST_DISK_TARGET)
	./$(TEST_DISK_TARGET)

test-buffer: $(TEST_BUFFER_TARGET)
	./$(TEST_BUFFER_TARGET)

test-lexer: $(TEST_LEXER_TARGET)
	./$(TEST_LEXER_TARGET)

test-parser: $(TEST_PARSER_TARGET)
	./$(TEST_PARSER_TARGET)

test-executor: $(TEST_EXECUTOR_TARGET)
	./$(TEST_EXECUTOR_TARGET)

test-database: $(TEST_DATABASE_TARGET)
	./$(TEST_DATABASE_TARGET)

test-stress: $(TEST_STRESS_TARGET)
	./$(TEST_STRESS_TARGET)

.PHONY: all clean test test-bpt test-disk test-buffer test-lexer test-parser test-executor test-database test-stress
