# Compiler
CC = gcc

# Executable names
TARGET = main
GUI_TARGET = gui

# Backend source files (excluding main.c for GUI)
SRC_DIR = src
COMMON_SRCS = $(SRC_DIR)/memory.c $(SRC_DIR)/process.c \
              $(SRC_DIR)/fileio.c $(SRC_DIR)/interpreter.c $(SRC_DIR)/mutex.c \
              $(SRC_DIR)/scheduler.c 

# Main CLI version
SRCS = $(SRC_DIR)/main.c $(COMMON_SRCS)

# GUI source files
GUI_SRCS = $(SRC_DIR)/gui_main.c 

# Build rules
all: $(TARGET)

$(TARGET):
	$(CC) $(SRCS) -o $(SRC_DIR)/$(TARGET)

gui: FORCE
	mkdir -p bin
	$(CC) $(GUI_SRCS) $(COMMON_SRCS) -o bin/$(GUI_TARGET) `pkg-config --cflags --libs gtk+-3.0`

clean:
	rm -f $(SRC_DIR)/$(TARGET) bin/$(GUI_TARGET)

FORCE:  # this will force make to always rebuild gui
