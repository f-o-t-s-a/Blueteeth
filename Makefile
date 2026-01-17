CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11 -D_POSIX_C_SOURCE=200809L `pkg-config --cflags dbus-1 glib-2.0`
LDFLAGS = `pkg-config --libs dbus-1 glib-2.0` -lpthread

# Directories
SRC_DIR = src
MAIN_SRC_DIR = $(SRC_DIR)/main
TEST_SRC_DIR = $(SRC_DIR)/test
INC_DIR = include
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/obj

# Auto-discover all include directories
INC_SUBDIRS = $(shell find $(INC_DIR) -type d 2>/dev/null)
INCLUDES = $(addprefix -I,$(INC_SUBDIRS))

# === SHARED/LIBRARY CODE (auto-discovered) ===
SHARED_SRCS = $(shell find $(MAIN_SRC_DIR) -type f -name '*.c' 2>/dev/null | grep -v '/main\.c$$')
SHARED_OBJS = $(patsubst $(MAIN_SRC_DIR)/%.c, $(OBJ_DIR)/main/%.o, $(SHARED_SRCS))

# === TEST EXECUTABLES (auto-discovered) ===
TEST_SRCS = $(wildcard $(TEST_SRC_DIR)/test_*.c)
TEST_TARGETS = $(patsubst $(TEST_SRC_DIR)/test_%.c, $(BIN_DIR)/test_%, $(TEST_SRCS))
TEST_OBJS = $(patsubst $(TEST_SRC_DIR)/%.c, $(OBJ_DIR)/test/%.o, $(TEST_SRCS))

# Extract test names for dynamic target generation
TEST_NAMES = $(patsubst $(TEST_SRC_DIR)/test_%.c, %, $(TEST_SRCS))

# === DEFAULT TARGET ===
all: $(TEST_TARGETS)
	@echo ""
	@echo "✓ Build complete!"
	@echo "Available executables:"
	@for target in $(TEST_TARGETS); do echo "  - $$target"; done

# === CREATE DIRECTORIES ===
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR)/test)
$(shell find $(MAIN_SRC_DIR) -type d 2>/dev/null | sed 's|$(MAIN_SRC_DIR)|$(OBJ_DIR)/main|' | xargs mkdir -p 2>/dev/null)

# === COMPILATION RULES ===

$(OBJ_DIR)/main/%.o: $(MAIN_SRC_DIR)/%.c
	@echo "Compiling shared: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/test/%.o: $(TEST_SRC_DIR)/%.c
	@echo "Compiling test: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# === LINKING RULES ===

$(BIN_DIR)/test_%: $(OBJ_DIR)/test/test_%.o $(SHARED_OBJS)
	@echo "Linking: $@"
	$(CC) $^ -o $@ $(LDFLAGS)

# === DYNAMIC TARGET GENERATION ===
# This generates actual targets for each discovered test

define make-test-targets
run-$(1): $(BIN_DIR)/test_$(1)
	@echo "Running test_$(1)..."
	@./$(BIN_DIR)/test_$(1)

sudo-$(1): $(BIN_DIR)/test_$(1)
	@echo "Running test_$(1) with sudo..."
	@sudo ./$(BIN_DIR)/test_$(1)

build-$(1): $(BIN_DIR)/test_$(1)
	@echo "Built test_$(1)"

clean-$(1):
	@echo "Cleaning test_$(1)..."
	@rm -f $(BIN_DIR)/test_$(1) $(OBJ_DIR)/test/test_$(1).o

rebuild-$(1): clean-$(1) build-$(1)
	@echo "Rebuilt test_$(1)"

.PHONY: run-$(1) sudo-$(1) build-$(1) clean-$(1) rebuild-$(1)
endef

# Generate targets for each test
$(foreach test,$(TEST_NAMES),$(eval $(call make-test-targets,$(test))))

# === GLOBAL CLEAN TARGETS ===

clean:
	@echo "Cleaning all build artifacts..."
	@rm -rf $(BIN_DIR)

clean-tests:
	@echo "Cleaning test executables..."
	@rm -f $(TEST_TARGETS) $(TEST_OBJS)

clean-shared:
	@echo "Cleaning shared objects..."
	@rm -f $(SHARED_OBJS)

# === REBUILD TARGETS ===

rebuild: clean all

rebuild-shared: clean-shared all

# === DEBUG BUILD ===

debug: CFLAGS += -DDEBUG -ggdb -O0
debug: clean all

# === INSTALL DEPENDENCIES ===

install-deps:
	sudo apt-get update
	sudo apt-get install -y libdbus-1-dev libglib2.0-dev pkg-config build-essential bluez libbluetooth-dev

# === INFORMATION TARGETS ===

list:
	@echo "Discovered test executables:"
	@for name in $(TEST_NAMES); do echo "  - test_$$name (use: make run-$$name or make sudo-$$name)"; done
	@echo ""
	@echo "Quick reference:"
	@echo "  make              - Build all tests"
	@echo "  make run-NAME     - Run test_NAME"
	@echo "  make sudo-NAME    - Run test_NAME with sudo"
	@echo "  make build-NAME   - Build test_NAME"
	@echo "  make clean-NAME   - Clean test_NAME"
	@echo "  make rebuild-NAME - Rebuild test_NAME"

show-shared:
	@echo "Shared/library sources (linked into all tests):"
	@for src in $(SHARED_SRCS); do echo "  - $$src"; done

show-tests:
	@echo "Test sources:"
	@for src in $(TEST_SRCS); do echo "  - $$src"; done

show-includes:
	@echo "Include paths:"
	@echo $(INCLUDES) | tr ' ' '\n'

info: list
	@echo ""
	@echo "Shared modules:"
	@for src in $(SHARED_SRCS); do echo "  - $$src"; done

help:
	@echo "Auto-discovering Bluetooth Project Build System"
	@echo ""
	@echo "This Makefile automatically discovers:"
	@echo "  • All test_*.c files in src/test/ → bin/test_*"
	@echo "  • All shared code in src/main/*/ → linked into tests"
	@echo ""
	@echo "Common commands:"
	@echo "  make                - Build all tests"
	@echo "  make list           - List all discovered tests"
	@echo "  make run-NAME       - Run test_NAME"
	@echo "  make sudo-NAME      - Run test_NAME with sudo"
	@echo "  make build-NAME     - Build test_NAME only"
	@echo "  make clean-NAME     - Clean test_NAME"
	@echo "  make rebuild-NAME   - Rebuild test_NAME"
	@echo "  make clean          - Clean everything"
	@echo "  make rebuild        - Rebuild everything"
	@echo ""
	@echo "Example workflow:"
	@echo "  1. Create src/test/test_routing.c"
	@echo "  2. Run: make build-routing"
	@echo "  3. Run: make sudo-routing"
	@echo "  4. No Makefile changes needed!"
	@echo ""
	@echo "Current tests:"
	@for name in $(TEST_NAMES); do echo "  • test_$$name"; done

.PHONY: all clean clean-tests clean-shared rebuild rebuild-shared debug \
        install-deps list show-shared show-tests show-includes info help
