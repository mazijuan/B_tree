CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c
TEST_DISK_SRCS = tests/test_disk_manager.c
TEST_BPT_SRCS = tests/test_bpt.c
OBJS = $(SRCS:.c=.o)
TEST_DISK_OBJS = $(TEST_DISK_SRCS:.c=.o)
TEST_BPT_OBJS = $(TEST_BPT_SRCS:.c=.o)

TARGET = libdbengine.a
TEST_DISK_TARGET = test_disk_manager
TEST_BPT_TARGET = test_bpt

all: $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(TEST_DISK_TARGET): $(TEST_DISK_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BPT_TARGET): $(TEST_BPT_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TEST_DISK_OBJS) $(TEST_BPT_OBJS) $(TARGET) $(TEST_DISK_TARGET) $(TEST_BPT_TARGET) *.db

test: $(TEST_DISK_TARGET) $(TEST_BPT_TARGET)
	@echo "=== Disk Manager Tests ==="
	./$(TEST_DISK_TARGET)
	@echo ""
	@echo "=== B+ Tree Tests ==="
	./$(TEST_BPT_TARGET)

test-bpt: $(TEST_BPT_TARGET)
	./$(TEST_BPT_TARGET)

test-disk: $(TEST_DISK_TARGET)
	./$(TEST_DISK_TARGET)

.PHONY: all clean test test-bpt test-disk
