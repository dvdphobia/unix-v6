# Unix V6 x86 Port - Main Makefile
# Modern OS-style build system

# Cross compiler
CC            = i686-elf-gcc
AS            = i686-elf-as
LD            = i686-elf-ld
OBJCOPY       = i686-elf-objcopy
GRUB_MKRESCUE = grub-mkrescue

# Directories
ARCH_DIR      = arch/x86
KERNEL_DIR    = kernel
INCLUDE_DIR   = include
BUILD_DIR     = build

# Flags
CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra
CFLAGS       += -I$(INCLUDE_DIR) -std=gnu99 -fno-builtin -fno-stack-protector
CFLAGS       += -Wno-unused-parameter -Wno-unused-variable
ASFLAGS       = --32
LDFLAGS       = -T $(ARCH_DIR)/linker.ld -nostdlib

# Source files
ASM_SRCS      = $(ARCH_DIR)/boot/boot.S
KERNEL_SRCS   = $(KERNEL_DIR)/core/main.c \
                $(KERNEL_DIR)/fs/bio.c \
                $(KERNEL_DIR)/fs/alloc.c \
                $(KERNEL_DIR)/drivers/block/ramdisk.c

# Object files
ASM_OBJS      = $(BUILD_DIR)/boot.o
KERNEL_OBJS   = $(BUILD_DIR)/main.o \
                $(BUILD_DIR)/bio.o \
                $(BUILD_DIR)/alloc.o \
                $(BUILD_DIR)/ramdisk.o

OBJS          = $(ASM_OBJS) $(KERNEL_OBJS)

# Output
KERNEL_ELF    = $(BUILD_DIR)/kernel.elf
ISO_FILE      = $(BUILD_DIR)/unix_v6.iso

.PHONY: all clean iso run help

all: $(KERNEL_ELF)

help:
	@echo "Unix V6 x86 Build System"
	@echo "========================"
	@echo ""
	@echo "  make          - Build kernel"
	@echo "  make iso      - Create bootable ISO"
	@echo "  make run      - Run in QEMU (serial console)"
	@echo "  make clean    - Clean build files"
	@echo ""

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile assembly
$(BUILD_DIR)/boot.o: $(ARCH_DIR)/boot/boot.S | $(BUILD_DIR)
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Compile C files
$(BUILD_DIR)/main.o: $(KERNEL_DIR)/core/main.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/bio.o: $(KERNEL_DIR)/fs/bio.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/alloc.o: $(KERNEL_DIR)/fs/alloc.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ramdisk.o: $(KERNEL_DIR)/drivers/block/ramdisk.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link kernel
$(KERNEL_ELF): $(OBJS)
	@echo "LD  $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo ""
	@echo "Kernel built: $@"

# Create bootable ISO
iso: $(KERNEL_ELF)
	@mkdir -p $(BUILD_DIR)/isodir/boot/grub
	@cp $(KERNEL_ELF) $(BUILD_DIR)/isodir/boot/kernel.elf
	@cp $(ARCH_DIR)/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	@$(GRUB_MKRESCUE) -o $(ISO_FILE) $(BUILD_DIR)/isodir 2>/dev/null
	@echo "ISO created: $(ISO_FILE)"

# Run in QEMU
run: iso
	@echo "Starting Unix V6 x86..."
	@echo "Press Ctrl-A X to exit"
	@echo ""
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -nographic

# Clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"
