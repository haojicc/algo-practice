# Makefile for algo-practice project
# Supports Linux and macOS, C11 standard.

CC = cc
BUILD ?= release
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic
ifeq ($(BUILD),debug)
	CFLAGS += -g -O0 -DDEBUG
else
	CFLAGS += -O2 -DNDEBUG
endif
TEST_CFLAGS = $(CFLAGS) -UNDEBUG
LDFLAGS =

MAIN_SOURCES = $(shell find src -name '*.c' | grep -vE 'src/tests/test_zset.c|src/tests/benchmark.c')
UTIL_SOURCES = $(shell find src/utils -name '*.c')
TARGET = main
TEST_TARGET = test_zset
BENCHMARK_TARGET = benchmark-bin

.PHONY: all clean run debug release test valgrind benchmark

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(MAIN_SOURCES)
	$(CC) $(CFLAGS) $(MAIN_SOURCES) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): src/tests/test_zset.c $(UTIL_SOURCES)
	$(CC) $(TEST_CFLAGS) src/tests/test_zset.c $(UTIL_SOURCES) -o $(TEST_TARGET) $(LDFLAGS)

$(BENCHMARK_TARGET): src/tests/benchmark.c $(UTIL_SOURCES)
	$(CC) $(TEST_CFLAGS) src/tests/benchmark.c $(UTIL_SOURCES) -o $(BENCHMARK_TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) BUILD=debug all

release:
	$(MAKE) BUILD=release all

test: $(TEST_TARGET)
	./$(TEST_TARGET)

valgrind: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_TARGET)

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(BENCHMARK_TARGET)
