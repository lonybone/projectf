CC = gcc

CFLAGS = -Wall -g -MMD -MP

TARGET = pf

SRCS = $(wildcard *.c)

OBJS = $(SRCS:.c=.o)

DEPS = $(SRCS:.c=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

-include $(DEPS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)

.PHONY: all clean
