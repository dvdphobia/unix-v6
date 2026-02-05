# CRT Makefile

CC = /opt/cross/bin/i686-elf-gcc
AR = /opt/cross/bin/i686-elf-ar
CFLAGS = -m32 -ffreestanding -nostdlib -O2 -Wall -Wextra -fno-builtin -fno-stack-protector

CRT_OBJS = crt0.o crt1.o crti.o crtn.o crtbegin.o crtend.o

all: $(CRT_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

.PHONY: all clean
