# Makefile

# Compiler
CC = gcc

# Executable name
TARGET = main2

# Source files (now inside src/)
SRCS = src/main2.c src/memory.c src/process.c src/fileio.c src/interpreter.c src/mutex.c src/scheduler.c

# Build rule
all:
	$(CC) $(SRCS) -o src/$(TARGET)

# Clean rule
clean:
	rm -f src/$(TARGET)
