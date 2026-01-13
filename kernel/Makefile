# Makefile - Unix V6 x86 Kernel Build Configuration
# Full kernel port from PDP-11 to x86

# Toolchain configuration
CC            = i686-elf-gcc
AS            = i686-elf-as
LD            = i686-elf-ld
OBJCOPY       = i686-elf-objcopy
GRUB_MKRESCUE = grub-mkrescue

# Compiler and assembler flags
CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra -Wno-unused-parameter
CFLAGS       += -I. -std=gnu99 -fno-builtin -fno-stack-protector
ASFLAGS       = --32
LDFLAGS       = -T linker.ld -nostdlib

# Source files - Assembly
ASM_SRCS      = x86.S
ASM_OBJS      = $(ASM_SRCS:.S=.o)

# Source files - C (ported from original V6)
C_SRCS        = main.c slp.c trap.c sysent.c bio.c alloc.c iget.c nami.c fio.c rdwri.c sig.c sys.c tty.c malloc.c clock.c pipe.c text.c drivers/block/ramdisk.c drivers/char/console.c
C_OBJS        = $(C_SRCS:.c=.o)

# All objects
OBJS          = $(ASM_OBJS) $(C_OBJS)

# Output files
KERNEL_ELF    = kernel.elf
KERNEL_BIN    = kernel.bin
ISO_FILE      = unix_v6.iso

# Phony targets (not actual files)
.PHONY: all clean build iso run run-vga debug help

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
build: $(KERNEL_ELF)

# Compile assembly files
%.o: %.S
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

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
iso: $(KERNEL_ELF)
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL_ELF) isodir/boot/kernel.elf
	@cp grub.cfg isodir/boot/grub/grub.cfg
	@$(GRUB_MKRESCUE) -o $(ISO_FILE) isodir 2>/dev/null
	@echo "ISO created: $(ISO_FILE)"

# Run in QEMU with serial output
run: iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -serial stdio -display none

# Run in QEMU with VGA display
run-vga: iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M

# Debug in QEMU with GDB
debug: iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -s -S &
	@echo "Connect with: gdb -ex 'target remote localhost:1234' $(KERNEL_ELF)"

# Clean build artifacts
clean:
	@rm -f $(OBJS) $(KERNEL_ELF) $(KERNEL_BIN) boot.o kernel.o
	@rm -rf isodir $(ISO_FILE)
	@echo "Clean complete."

# Dependencies
main.o: main.c include/types.h include/param.h include/user.h include/systm.h
slp.o: slp.c include/types.h include/param.h include/user.h include/proc.h
trap.o: trap.c include/types.h include/param.h include/user.h include/reg.h
sysent.o: sysent.c include/types.h include/param.h include/user.h

.PRECIOUS: $(OBJS)
