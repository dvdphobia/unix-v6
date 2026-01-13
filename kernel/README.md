# Unix V6 x86 Kernel Project Scaffold - Complete Build Documentation

## Project Overview

This directory contains a complete, bootable kernel scaffold for porting the original Research Unix V6 (1975) to modern x86 architecture. The project is designed to compile on modern Linux/Unix systems using a cross-compiler toolchain.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    GRUB Bootloader                          │
│                   (Multiboot v1)                            │
└───────────────┬─────────────────────────────────────────────┘
                │
                ↓
        ┌───────────────┐
        │   boot.S      │  (Assembly Entry Point)
        │  (GAS/x86)    │  - Sets up kernel stack
        │               │  - Disables interrupts
        │               │  - Calls kmain()
        └───────┬───────┘
                │
                ↓
        ┌───────────────┐
        │   kernel.c    │  (Modern C Entry Point)
        │  (C99)        │  - VGA buffer initialization
        │               │  - Placeholder for V6 subsystems
        └───────────────┘
```

## Files in This Project

### 1. **boot.S** - Assembly Entry Point
- **Purpose**: Initial kernel entry point called by GRUB
- **Key Functions**:
  - Validates Multiboot header
  - Establishes kernel stack (16 KB)
  - Disables interrupts (CLI)
  - Jumps to `kmain()` C function
  - Contains infinite halt loop for safety

### 2. **kernel.c** - Modernized Kernel Entry Point
- **Purpose**: Modern C99 replacement for original V6 `main.c`
- **Key Functions**:
  - `vga_putchar()` - Write single character to VGA buffer
  - `vga_write_string()` - Write null-terminated string
  - `kmain()` - Main kernel initialization
- **Features**:
  - VGA text mode console (80x25 characters)
  - White-on-black text output
  - Comprehensive placeholder comments for V6 subsystem integration

### 3. **linker.ld** - Linker Configuration
- **Purpose**: Defines memory layout and section organization
- **Key Features**:
  - Links kernel at 1MB (0x100000) - standard x86 kernel address
  - Multiboot header positioned at beginning of .text
  - Separate sections for code, data, and BSS
  - 4KB section alignment

### 4. **Makefile** - Build System
- **Purpose**: Automates compilation and linking
- **Targets**:
  - `make build` - Compile and link kernel ELF
  - `make iso` - Generate bootable ISO image
  - `make clean` - Remove build artifacts
  - `make help` - Display available targets

### 5. **grub.cfg** - GRUB Configuration
- **Purpose**: GRUB bootloader menu configuration
- **Features**:
  - Multiboot loader directive
  - Boot entry for Unix V6 x86 kernel

### 6. **setup-unix-v6-build.sh** - Automated Setup
- **Purpose**: Installs all required cross-compilation tools
- **Installs**:
  - i686-elf-gcc cross-compiler
  - GNU Assembler (GAS) for x86
  - GNU Binutils (ld, objcopy, etc.)
  - GRUB utilities for ISO generation

## Installation and Build Instructions

### Step 1: Install Dependencies

```bash
# Make setup script executable
chmod +x /workspaces/unix-v6/kernel/setup-unix-v6-build.sh

# Run the automated setup (requires sudo)
/workspaces/unix-v6/kernel/setup-unix-v6-build.sh
```

**Manual installation (if preferred):**
```bash
# Update package manager
sudo apt-get update

# Install cross-compiler and build tools
sudo apt-get install -y \
    build-essential \
    gcc-multilib \
    g++-multilib \
    binutils \
    bison \
    flex \
    wget \
    curl

# Install GRUB utilities
sudo apt-get install -y \
    grub-common \
    grub-pc \
    grub-pc-bin \
    xorriso
```

### Step 2: Build the Kernel

```bash
# Navigate to kernel directory
cd /workspaces/unix-v6/kernel

# Clean any previous builds
make clean

# Compile and link the kernel
make build

# Verify the ELF binary was created
ls -lh kernel.elf
```

### Step 3: Generate Bootable ISO

```bash
# Create bootable ISO image
make iso

# Verify ISO was created
ls -lh unix_v6.iso
```

### Step 4: Test with QEMU (Optional)

If you have QEMU installed, you can test the kernel:

```bash
# Install QEMU (if not already installed)
sudo apt-get install -y qemu-system-x86

# Boot the ISO
qemu-system-i386 -cdrom unix_v6.iso
```

## Expected Output

When booted, the kernel should display:
```
Unix V6 x86 Port Loading...
Initializing process table...
Initializing inode buffer pool...
Initializing buffer cache...
Initializing file descriptor table...
Initializing TTY subsystem...
Mounting root filesystem...
Setting up interrupt handlers...
Spawning init process...
Kernel initialization complete.
```

Then the kernel enters an infinite halt loop, waiting for interrupt-driven multitasking to be implemented.

## Integration Points for Original V6 Code

The `kernel.c` file contains placeholder TODO comments at key integration points:

1. **Process Table** - `proc[]` array initialization
2. **Inode Cache** - Buffer pool for filesystem metadata
3. **Buffer Cache** - Disk I/O buffering system
4. **File Descriptor Table** - Per-process file handling
5. **TTY Subsystem** - Terminal driver initialization
6. **Filesystem** - Root device mounting
7. **Interrupt Handlers** - Trap/system call vector setup
8. **Scheduler** - Process initialization and multitasking

Each section includes references to the original V6 header files and structures from the `sys/` directory.

## Cross-Compiler Toolchain Details

### i686-elf-gcc
- **Target**: i686 (32-bit x86) ELF executable format
- **Host**: x86_64 Linux (or ARM64 with multilib)
- **Flags**:
  - `-ffreestanding`: Compiles without standard library
  - `-O2`: Optimization level 2
  - `-nostdlib`: Don't link standard C library
  - `-Wall -Wextra`: Enable all warnings

### GNU Assembler (GAS)
- **Format**: AT&T syntax (default for GAS)
- **Architecture**: 32-bit x86
- **Flag**: `--32` (explicitly target 32-bit i386)

### GNU Linker (ld)
- **Script**: `linker.ld` defines memory layout
- **Entry**: `_start` symbol in `boot.S`
- **Sections**: `.text`, `.data`, `.bss`

## Multiboot v1 Specification Compliance

This kernel follows the Multiboot v1 specification:

- **Magic Number**: `0x1BADBABE` identifies Multiboot header
- **Header Checksum**: Sum of magic, flags, and checksum must equal 0
- **Flags**: `0x00000003` (bits 0-1 set for standard features)
- **Placement**: First 8KB of kernel image

GRUB will:
1. Load the kernel at 1MB (0x100000)
2. Verify Multiboot header
3. Set up protected mode (32-bit)
4. Jump to `_start` symbol
5. Pass boot information in `EBX` register (not used in this simple scaffold)

## Memory Layout

```
Virtual Address Space (x86 32-bit):
┌──────────────────────────────┐
│     Kernel Space             │
│   (1MB + size)               │
├──────────────────────────────┤
│     Kernel Code (.text)      │ ← 0x100000 (1MB)
├──────────────────────────────┤
│     Kernel Data (.data)      │
├──────────────────────────────┤
│     Kernel BSS (.bss)        │
├──────────────────────────────┤
│     Kernel Stack             │
│   (16 KB, grows downward)    │
└──────────────────────────────┘
```

## Next Steps for V6 Integration

1. **Filesystem Driver**: Port V6's `ufs` filesystem driver
2. **Process Management**: Implement process table and scheduler
3. **Interrupt Handling**: Set up IDT and ISR handlers
4. **Memory Management**: Implement paging and virtual memory
5. **Device Drivers**: Port TTY, disk I/O drivers
6. **System Calls**: Implement Unix system call interface
7. **Shell/Init**: Port V6's `init` and shell programs

## Troubleshooting

### "i686-elf-gcc: command not found"
- Install cross-compiler: `sudo apt-get install gcc-multilib`
- Or run setup script: `/workspaces/unix-v6/kernel/setup-unix-v6-build.sh`

### "grub-mkrescue: command not found"
- Install GRUB utilities: `sudo apt-get install grub-pc-bin xorriso`

### "make: *** [kernel.elf] Error 1"
- Check linker script path is correct
- Verify `linker.ld` syntax with: `ld -T linker.ld --verbose`

### ISO won't boot
- Verify Multiboot header with: `file kernel.elf`
- Test with: `qemu-system-i386 -cdrom unix_v6.iso -d int`

## References

- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [OSDev.org Wiki](https://wiki.osdev.org/)
- [x86 Assembly Language](https://www.intel.com/content/dam/develop/external/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf)
- [Unix V6 Source Code](/workspaces/unix-v6/source/)

## License

This scaffold is provided as-is for educational purposes. Original Unix V6 code is subject to its original licensing terms.

---

**Created**: 2026
**Target**: i686-elf (32-bit x86)
**Boot Method**: Multiboot v1 (GRUB)
