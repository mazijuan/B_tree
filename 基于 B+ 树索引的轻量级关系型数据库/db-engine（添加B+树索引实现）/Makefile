CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c src/bplus_tree.c
TEST_SRCS_DISK = tests/test_disk_manager.c
TEST_SRCS_BPT = tests/test_bpt.c
OBJS = $(SRCS:.c=.o)

TARGET = libdbengine.a
TEST_TARGET_DISK = test_disk_manager
TEST_TARGET_BPT = test_bpt

all: $(TARGET) $(TEST_TARGET_DISK) $(TEST_TARGET_BPT)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(TEST_TARGET_DISK): $(TEST_SRCS_DISK) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET_BPT): $(TEST_SRCS_BPT) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET_DISK) $(TEST_TARGET_BPT) *.db

test-disk: $(TEST_TARGET_DISK)
	./$(TEST_TARGET_DISK)

test-bpt: $(TEST_TARGET_BPT)
	./$(TEST_TARGET_BPT)

test: test-disk test-bpt

.PHONY: all clean test
