# Applications Makefile
# Builds all applications in the Applications folder

# Variables
BUILD_DIR     = ../../build/userland
CC            = i686-elf-gcc
LD            = i686-elf-ld
OBJCOPY       = i686-elf-objcopy

CFLAGS        = -ffreestanding -O2 -nostdlib -Wall -Wextra
CFLAGS       += -I.. -I../libc -fno-builtin -fno-stack-protector
LDFLAGS       = -T ../linker.ld -nostdlib

# CRT and libc from parent
LIBC          = $(BUILD_DIR)/libc.a
CRT_START     = $(BUILD_DIR)/crt0.o $(BUILD_DIR)/crti.o $(BUILD_DIR)/crtbegin.o
CRT_END       = $(BUILD_DIR)/crtend.o $(BUILD_DIR)/crtn.o

# Applications
APPS          = terminal

# Source files for each application
TERMINAL_SRC  = Terminal/terminal.c

.PHONY: all clean

all: $(addprefix $(BUILD_DIR)/,$(addsuffix .bin,$(APPS)))

# Build Terminal application
$(BUILD_DIR)/terminal.o: $(TERMINAL_SRC) ../syscalls.h | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Link each application with full CRT and libc
$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o $(CRT_START) $(BUILD_DIR)/syscalls.o $(LIBC) $(CRT_END) ../linker.ld
	@echo "LD  $@"
	@$(LD) -T ../linker.ld -nostdlib -o $@ \
		$(CRT_START) \
		$(BUILD_DIR)/syscalls.o \
		$(BUILD_DIR)/$*.o \
		$(LIBC) \
		$(CRT_END)

# Convert ELF to flat binary
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "BIN $@"
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -f $(BUILD_DIR)/terminal.o $(BUILD_DIR)/terminal.elf $(BUILD_DIR)/terminal.bin
	@echo "Applications clean complete."
