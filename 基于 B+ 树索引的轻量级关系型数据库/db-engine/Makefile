CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = 

SRCS = src/disk_manager.c
TEST_SRCS = tests/test_disk_manager.c
OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)

TARGET = libdbengine.a
TEST_TARGET = test_disk_manager

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(TEST_TARGET): $(TEST_OBJS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET) *.db

test: $(TEST_TARGET)
	./$(TEST_TARGET)

.PHONY: all clean test
