# Makefile for userland libc

CC = /opt/cross/bin/i686-elf-gcc
AR = /opt/cross/bin/i686-elf-ar
CFLAGS = -m32 -ffreestanding -nostdlib -O2 -Wall -Wextra -I../ -fno-builtin -fno-stack-protector

LIBC_OBJS = string.o stdlib.o stdio.o ctype.o dirent.o errno.o
CRT_DIR = crt
CRT_OBJS = $(CRT_DIR)/crt0.o $(CRT_DIR)/crt1.o $(CRT_DIR)/crti.o \
           $(CRT_DIR)/crtn.o $(CRT_DIR)/crtbegin.o $(CRT_DIR)/crtend.o

all: libc.a crt

libc.a: $(LIBC_OBJS)
	$(AR) rcs libc.a $(LIBC_OBJS)

crt:
	$(MAKE) -C $(CRT_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(CRT_DIR) clean
	rm -f *.o libc.a

.PHONY: all clean crt
