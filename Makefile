# Unix V6 x86 Port - Root Makefile
# Orchestrates userland build, ramdisk generation, and kernel build

KERNEL_DIR := kernel
USER_DIR   := userland
BUILD_DIR  := build
MANIFEST   := $(USER_DIR)/ramdisk.manifest
RAMDISK_H  := $(KERNEL_DIR)/drivers/block/ramdisk_files.h
GEN_SCRIPT := tools/gen_ramdisk_files.py
TOOLCHAIN_CHECK := tools/ensure_toolchain.sh
TOOLCHAIN_PATH  := /opt/cross/bin

export PATH := $(TOOLCHAIN_PATH):$(PATH)

.PHONY: all userland ramdisk kernel iso run clean help toolchain

all: toolchain userland ramdisk kernel

help:
	@echo "Unix V6 x86 Build System"
	@echo "========================"
	@echo ""
	@echo "  make            - Build userland + kernel"
	@echo "  make iso        - Create bootable ISO"
	@echo "  make run        - Run in QEMU (serial)"
	@echo "  make clean      - Clean build artifacts"
	@echo "  make toolchain  - Check/install cross toolchain"
	@echo ""

userland:
	@$(MAKE) -C $(USER_DIR)

ramdisk: userland
	@python3 $(GEN_SCRIPT) --manifest $(MANIFEST) --output $(RAMDISK_H)

kernel: toolchain ramdisk
	@$(MAKE) -C $(KERNEL_DIR) build

iso: kernel
	@$(MAKE) -C $(KERNEL_DIR) iso

run: iso
	@$(MAKE) -C $(KERNEL_DIR) run

clean:
	@$(MAKE) -C $(KERNEL_DIR) clean
	@$(MAKE) -C $(USER_DIR) clean
	@rm -rf $(BUILD_DIR)/userland
	@echo "Clean complete."

toolchain:
	@./$(TOOLCHAIN_CHECK)
