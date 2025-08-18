# Compiler to use
CC = gcc

# Compiler flags:
# -Wall = Show all common warnings
# -g    = Include debugging information
CFLAGS = -Wall -g

# The name of your final executable program
TARGET = pf

# List of your source (.c) files
SRCS = main.c lexer.c

# Automatically generate a list of object (.o) files from the source files
OBJS = $(SRCS:.c=.o)

# The default rule, which is run when you just type "make"
# This rule says that the "all" goal depends on the TARGET executable.
all: $(TARGET)

# Rule to link the object files into the final executable
# This depends on all the object files.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pattern rule to compile a .c file into a .o file
# This says that to make any .o file, you need its corresponding .c file and lexer.h.
# If lexer.h changes, all .c files will be recompiled.
%.o: %.c lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up the directory
# This removes all compiled object files and the final executable.
clean:
	rm -f $(OBJS) $(TARGET)

# Tells make that "all" and "clean" are not actual files.
.PHONY: all clean
