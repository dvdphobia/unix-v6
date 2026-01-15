# Makefile - Unix V6 x86 Kernel Build Configuration
# Full kernel port from PDP-11 to x86

# Toolchain configuration
TOOLCHAIN_CHECK = ../tools/ensure_toolchain.sh
TOOLCHAIN_PATH  = /opt/cross/bin

export PATH := $(TOOLCHAIN_PATH):$(PATH)

CC            = i686-elf-gcc
AS            = i686-elf-as
LD            = i686-elf-ld
OBJCOPY       = i686-elf-objcopy
GRUB_MKRESCUE = grub-mkrescue

# Compiler and assembler flags
CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra -Wno-unused-parameter
CFLAGS       += -I. -I.. -std=gnu99 -fno-builtin -fno-stack-protector
ASFLAGS       = --32
LDFLAGS       = -T linker.ld -nostdlib

# Source files - Assembly
ASM_SRCS      = x86.S
ASM_OBJS      = $(ASM_SRCS:.S=.o)

# Source files - C (ported from original V6)
# Kernel core
KERNEL_SRCS   = main.c shell.c slp.c trap.c sysent.c bio.c alloc.c iget.c nami.c fio.c rdwri.c sig.c sys.c tty.c malloc.c clock.c pipe.c text.c
# Drivers
DRIVER_SRCS   = drivers/block/ramdisk.c drivers/block/ide.c drivers/char/console.c
# Built-in commands (from /bin/builtin)
BUILTIN_SRCS  = ../bin/builtin/ls.c ../bin/builtin/cd.c ../bin/builtin/pwd.c ../bin/builtin/echo.c ../bin/builtin/ps.c ../bin/builtin/uname.c ../bin/builtin/cat.c

C_SRCS        = $(KERNEL_SRCS) $(DRIVER_SRCS) $(BUILTIN_SRCS)
C_OBJS        = $(C_SRCS:.c=.o)

# All objects
OBJS          = $(ASM_OBJS) $(C_OBJS)

# Output files
KERNEL_ELF    = kernel.elf
KERNEL_BIN    = kernel.bin
ISO_FILE      = unix_v6.iso
RAMDISK_H     = drivers/block/ramdisk_files.h
RAMDISK_GEN   = ../tools/gen_ramdisk_files.py
RAMDISK_MAN   = ../userspace/ramdisk.manifest

# Phony targets (not actual files)
.PHONY: all clean build iso run run-vga debug help userspace toolchain

# Default target
all: $(KERNEL_ELF)

# Display help information
help:
	@echo "Unix V6 x86 Kernel Build System"
	@echo "================================"
	@echo ""
	@echo "Targets:"
	@echo "  make build      - Build the kernel ELF"
	@echo "  make iso        - Create bootable ISO image"
	@echo "  make run        - Run kernel in QEMU (serial)"
	@echo "  make run-vga    - Run kernel in QEMU (VGA)"
	@echo "  make debug      - Run kernel in QEMU with GDB"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make help       - Display this help message"

# Build target (alias for all)
build: toolchain $(KERNEL_ELF)

# Build userspace before generating the ramdisk header
userspace:
	@$(MAKE) -C ../userspace

# Compile assembly files
%.o: %.S
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Generate ramdisk header (from userspace manifest)
$(RAMDISK_H): userspace $(RAMDISK_MAN) $(RAMDISK_GEN)
	@echo "GEN $@"
	@python3 $(RAMDISK_GEN) --manifest $(RAMDISK_MAN) --output $(RAMDISK_H)

# Compile C files
%.o: %.c
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link kernel
$(KERNEL_ELF): $(OBJS)
	@echo "LD  $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Kernel built: $(KERNEL_ELF)"

# Create bootable ISO image using GRUB
iso: toolchain $(KERNEL_ELF)
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL_ELF) isodir/boot/kernel.elf
	@cp grub.cfg isodir/boot/grub/grub.cfg
	@$(GRUB_MKRESCUE) -o $(ISO_FILE) isodir 2>/dev/null
	@echo "ISO created: $(ISO_FILE)"

# Run in QEMU with serial output
run: toolchain iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -serial stdio -display none

# Run in QEMU with VGA display
run-vga: toolchain iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M

# Debug in QEMU with GDB
debug: toolchain iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -s -S &
	@echo "Connect with: gdb -ex 'target remote localhost:1234' $(KERNEL_ELF)"

# Clean build artifacts
clean:
	@rm -f $(OBJS) $(KERNEL_ELF) $(KERNEL_BIN) boot.o kernel.o
	@rm -f ../bin/builtin/*.o
	@rm -f cmd/*.o
	@rm -f 00_START_HERE.txt COMMANDS.txt DELIVERY_SUMMARY.txt FILE_INDEX.txt INDEX.txt PROJECT_SUMMARY.txt START_HERE.txt
	@rm -rf isodir $(ISO_FILE) $(RAMDISK_H)
	@echo "Clean complete."

# Dependencies
main.o: main.c include/types.h include/param.h include/user.h include/systm.h
slp.o: slp.c include/types.h include/param.h include/user.h include/proc.h
trap.o: trap.c include/types.h include/param.h include/user.h include/reg.h
sysent.o: sysent.c include/types.h include/param.h include/user.h
drivers/block/ramdisk.o: drivers/block/ramdisk.c $(RAMDISK_H)

.PRECIOUS: $(OBJS)
# Check toolchain before build/run
toolchain:
	@$(TOOLCHAIN_CHECK)
