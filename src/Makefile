# Compiler and flags
CC = gcc
# Added -fsanitize=address to enable the AddressSanitizer for memory error detection
CFLAGS = -Wall -g -MMD -MP -fsanitize=address

# Project name
TARGET = pf

# Build directory
BUILD_DIR = build

# Source files
SRCS = $(wildcard *.c)

# Object files: place them in the BUILD_DIR
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

# Dependency files: derive from the new OBJS path
DEPS = $(OBJS:.o=.d)

# Default target
all: $(TARGET)

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Linking rule: depends on object files
# The sanitizer flag is also needed at link time.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation rule: compile .c files into the BUILD_DIR
# The '| $(BUILD_DIR)' is an order-only prerequisite that ensures the directory exists
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Include generated dependency files
-include $(DEPS)

# Clean rule: remove the target and the entire build directory
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
