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
LDFLAGS =

SOURCES = $(shell find src utils -name '*.c')
TARGET = main

.PHONY: all clean run debug release test valgrind

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) BUILD=debug all

release:
	$(MAKE) BUILD=release all

test: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

clean:
	rm -f $(TARGET)
