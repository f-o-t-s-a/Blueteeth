CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11 -D_POSIX_C_SOURCE=200809L `pkg-config --cflags dbus-1 glib-2.0`
LDFLAGS = `pkg-config --libs dbus-1 glib-2.0` -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

# Find all source files recursively in src/
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Generate object file paths in bin/obj/ maintaining directory structure
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/obj/%.o, $(SRCS))

# Get all subdirectories under include/
INC_SUBDIRS = $(shell find $(INC_DIR) -type d)

# Create proper include flags
INCLUDES = $(addprefix -I,$(INC_SUBDIRS))

# Target executable
TARGET = $(BIN_DIR)/bt-scanner

# Default target
all: $(TARGET)

# Create necessary directories
$(shell mkdir -p $(BIN_DIR) $(BIN_DIR)/obj $(shell find $(SRC_DIR) -type d | sed 's|$(SRC_DIR)|$(BIN_DIR)/obj|'))

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files with directory structure preservation
$(BIN_DIR)/obj/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)

# Run the program (requires sudo for Bluetooth)
run: $(TARGET)
	sudo $(TARGET)

# Debug build with more flags
debug: CFLAGS += -DDEBUG -ggdb -O0
debug: clean $(TARGET)

# Install dependencies
install-deps:
	sudo apt-get update
	sudo apt-get install -y libdbus-1-dev libglib2.0-dev pkg-config build-essential bluez libbluetooth-dev

# Show include paths being used
show-includes:
	@echo "Include paths:"
	@echo $(INCLUDES) | tr ' ' '\n'

.PHONY: all clean run debug install-deps show-includes
