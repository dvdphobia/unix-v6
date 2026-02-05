# Makefile for userland libc

CC = i686-elf-gcc
AR = i686-elf-ar
# Add -Iinclude for headers
CFLAGS = -m32 -ffreestanding -nostdlib -O2 -Wall -Wextra -Iinclude -fno-builtin -fno-stack-protector

SRC_DIR = src
CRT_DIR = src/crt

# Explicit list or wildcard for sources in src/
LIBC_SOURCES = $(wildcard $(SRC_DIR)/*.c)
LIBC_OBJS = $(LIBC_SOURCES:.c=.o)

all: libc.a crt

libc.a: $(LIBC_OBJS)
	$(AR) rcs libc.a $(LIBC_OBJS)

crt:
	$(MAKE) -C $(CRT_DIR) CC=$(CC) CFLAGS="$(CFLAGS) -I../../include"

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(CRT_DIR) clean
	rm -f $(SRC_DIR)/*.o libc.a

.PHONY: all clean crt
