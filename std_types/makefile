# Works with GNU Make on Linux, macOS, and Windows (somewhat)

ifeq ($(OS),Windows_NT)
    TARGET := std_types_test.exe
    RUN   := $(TARGET)
else
    TARGET := std_types_test
    RUN   := ./$(TARGET)
endif

CC ?= cc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

SRC = std_types_test.c

.PHONY: all check clean test

all: $(TARGET)

$(TARGET): $(SRC) std_types.h
	$(CC) $(CFLAGS) -o $@ $<

check: $(TARGET)
	$(RUN)
	@echo "Test passed."

clean:
ifeq ($(OS),Windows_NT)
	-powershell -NoProfile -Command "Remove-Item -ErrorAction SilentlyContinue $(TARGET)"
else
	-rm -f $(TARGET)
endif

test: clean all check
