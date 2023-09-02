TARGET = fpga_rbf_load

SRCS=$(wildcard *.c functions/*.c)
OBJECTS=$(SRCS:.c=.o)

## ALTERA INFO
ALT_DEVICE_FAMILY ?= soc_cv_av
HWLIBS_ROOT = $(SOCEDS_DEST_ROOT)/ip/altera/hps/altera_hps/hwlib

## COMPILER FLAGS
CFLAGS = -g -std=c99 -Wall -Werror -I$(HWLIBS_ROOT)/include -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -D$(ALT_DEVICE_FAMILY)
LDFLAGS = -g -std=c99 -Wall -Werror -pthread -lpthread

CROSS_COMPILE = arm-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
ARCH = arm

.PHONY: build
build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS)   $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~