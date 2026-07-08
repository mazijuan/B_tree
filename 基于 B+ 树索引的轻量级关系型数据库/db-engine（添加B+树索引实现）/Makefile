CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c src/buffer_pool.c
TEST_SRCS_DISK = tests/test_disk_manager.c
TEST_SRCS_BPT = tests/test_bpt.c
TEST_SRCS_BPT_DEL = tests/test_bpt_delete.c
TEST_SRCS_BP = tests/test_buffer_pool.c
OBJS = $(SRCS:.c=.o)

TARGET = libdbengine.a
TEST_TARGET_DISK = test_disk_manager
TEST_TARGET_BPT = test_bpt
TEST_TARGET_BPT_DEL = test_bpt_delete
TEST_TARGET_BP = test_buffer_pool

all: $(TARGET) $(TEST_TARGET_DISK) $(TEST_TARGET_BPT) $(TEST_TARGET_BPT_DEL) $(TEST_TARGET_BP)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(TEST_TARGET_DISK): $(TEST_SRCS_DISK) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET_BPT): $(TEST_SRCS_BPT) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET_BPT_DEL): $(TEST_SRCS_BPT_DEL) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET_BP): $(TEST_SRCS_BP) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET_DISK) $(TEST_TARGET_BPT) $(TEST_TARGET_BPT_DEL) $(TEST_TARGET_BP) *.db *.exe

test-disk: $(TEST_TARGET_DISK)
	./$(TEST_TARGET_DISK)

test-bpt: $(TEST_TARGET_BPT)
	./$(TEST_TARGET_BPT)

test-bpt-delete: $(TEST_TARGET_BPT_DEL)
	./$(TEST_TARGET_BPT_DEL)

test-bp: $(TEST_TARGET_BP)
	./$(TEST_TARGET_BP)

test: test-disk test-bpt test-bpt-delete test-bp

.PHONY: all clean test
