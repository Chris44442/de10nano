TARGET = fpga_config_tool

SRCS=$(wildcard *.c functions/*.c)
OBJECTS=$(SRCS:.c=.o)

CFLAGS = -g -std=c99 -Wall -Werror -O3
LDFLAGS = -g -std=c99 -Wall -Werror -pthread -lpthread

CROSS_COMPILE = arm-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

.PHONY: build
build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~