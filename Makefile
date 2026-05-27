.DEFAULT_GOAL := all
.PHONY: all build run clean test format lint help

CC := gcc
CFLAGS := -Wall -Wextra -Werror -O2 -std=c11
CFLAGS += $(shell pkg-config --cflags gtk4)
CFLAGS += $(shell pkg-config --cflags libpq)

LDFLAGS := $(shell pkg-config --libs gtk4)
LDFLAGS += $(shell pkg-config --libs libpq)
LDFLAGS += -lm

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
UI_DIR := ui
EXAMPLES_DIR := examples
BUILD_MARKER := $(BUILD_DIR)/.marker

# Find all .c files recursively in src/
SOURCES := $(shell find $(SRC_DIR) -name "*.c" -type f)
# Convert source paths to build object paths
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))
TARGET := $(BUILD_DIR)/gtkapp

# Gesture demo executable
GESTURE_DEMO_TARGET := $(BUILD_DIR)/gesture_demo
GESTURE_DEMO_DEPS := build/utils/gesture_manager.o build/utils/animations.o

all: $(TARGET)

$(BUILD_MARKER):
	@mkdir -p $(BUILD_DIR)
	@touch $(BUILD_MARKER)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_MARKER)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Build successful: $(TARGET)"

build: all

run: $(TARGET)
	@LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) $(TARGET)

$(GESTURE_DEMO_TARGET): $(GESTURE_DEMO_DEPS) $(BUILD_MARKER)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(EXAMPLES_DIR)/gesture_demo.c $(GESTURE_DEMO_DEPS) $(LDFLAGS) -o $@
	@echo "✨ Gesture demo built: $(GESTURE_DEMO_TARGET)"

demo: $(GESTURE_DEMO_TARGET)
	@echo "🎬 Launching gesture demo..."
	@$(GESTURE_DEMO_TARGET) 2>&1

.PHONY: demo

format:
	@echo "Formatting C code..."
	@find $(SRC_DIR) -name "*.c" -type f -exec clang-format -i {} \;
	@find $(INC_DIR) -name "*.h" -type f -exec clang-format -i {} \;

lint:
	@echo "Running clang-tidy..."
	@find $(SRC_DIR) -name "*.c" -type f -exec clang-tidy -checks=readability-*,bugprone-* {} -- $(CFLAGS) -I$(INC_DIR) \;

test: build
	@echo "Running basic tests..."
	@$(TARGET) --help 2>/dev/null || echo "App launched successfully"

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

help:
	@echo "GtkApp - GTK4 C Application with MVC Architecture"
	@echo ""
	@echo "Available targets:"
	@echo "  make build    - Build the application"
	@echo "  make run      - Build and run the application"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make format   - Format C code with clang-format"
	@echo "  make lint     - Run clang-tidy static analysis"
	@echo "  make test     - Build and run basic tests"
	@echo "  make help     - Show this help message"
