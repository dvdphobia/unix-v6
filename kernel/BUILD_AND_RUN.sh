#!/bin/bash
# ==============================================================================
# UNIX V6 X86 KERNEL - COMPLETE SETUP AND BUILD GUIDE
# ==============================================================================
# This file contains all the exact shell commands needed to get started.
# Copy and paste each section into your terminal.
#
# Target System: Ubuntu 24.04 LTS / Debian-based Linux
# Kernel Target: i686-elf (32-bit x86)
# Bootloader: GRUB (Multiboot v1)
# ==============================================================================

# ==============================================================================
# SECTION 1: INSTALL DEPENDENCIES
# ==============================================================================
# Run this entire block to install all required tools:

cat << 'EOF'
Installing build dependencies...
(This will prompt for sudo password)
EOF

sudo apt-get update && sudo apt-get install -y \
    build-essential \
    gcc-multilib \
    g++-multilib \
    binutils \
    bison \
    flex \
    wget \
    curl \
    git \
    grub-common \
    grub-pc \
    grub-pc-bin \
    xorriso \
    qemu-system-x86

echo "Dependency installation complete!"

# ==============================================================================
# SECTION 2: VERIFY TOOLCHAIN INSTALLATION
# ==============================================================================
# Run this block to verify all tools are available:

echo ""
echo "Verifying cross-compiler toolchain..."
echo ""

# Check each required tool
for tool in i686-elf-gcc i686-elf-as i686-elf-ld i686-elf-objcopy grub-mkrescue; do
    if command -v $tool &> /dev/null; then
        echo "✓ $tool is installed"
        $tool --version 2>/dev/null | head -1
    else
        echo "✗ $tool is NOT installed"
    fi
done

echo ""

# ==============================================================================
# SECTION 3: NAVIGATE TO PROJECT DIRECTORY
# ==============================================================================

PROJECT_DIR="/workspaces/unix-v6/kernel"
cd "$PROJECT_DIR"
echo "Working directory: $(pwd)"

# ==============================================================================
# SECTION 4: CLEAN PREVIOUS BUILDS (if any)
# ==============================================================================

echo ""
echo "Cleaning previous build artifacts..."
make clean
echo "Clean complete!"

# ==============================================================================
# SECTION 5: BUILD THE KERNEL
# ==============================================================================

echo ""
echo "Building Unix V6 x86 kernel..."
echo ""

make build

if [ -f kernel.elf ]; then
    echo ""
    echo "✓ Kernel build successful!"
    echo "  Output file: kernel.elf"
    ls -lh kernel.elf
    echo ""
    echo "ELF file information:"
    file kernel.elf
else
    echo ""
    echo "✗ Kernel build FAILED!"
    echo "  Check error messages above."
    exit 1
fi

# ==============================================================================
# SECTION 6: GENERATE BOOTABLE ISO IMAGE
# ==============================================================================

echo ""
echo "Generating bootable ISO image..."
echo ""

make iso

if [ -f unix_v6.iso ]; then
    echo ""
    echo "✓ ISO generation successful!"
    echo "  Output file: unix_v6.iso"
    ls -lh unix_v6.iso
    echo ""
    echo "ISO file information:"
    file unix_v6.iso
else
    echo ""
    echo "✗ ISO generation FAILED!"
    echo "  Check error messages above."
    exit 1
fi

# ==============================================================================
# SECTION 7: TEST WITH QEMU (Optional)
# ==============================================================================

echo ""
echo "To test the kernel with QEMU, run:"
echo ""
echo "  qemu-system-i386 -cdrom $PROJECT_DIR/unix_v6.iso"
echo ""
echo "You should see:"
echo "  1. GRUB boot menu"
echo "  2. Select 'Unix V6 x86' entry"
echo "  3. White text showing initialization messages"
echo ""

# ==============================================================================
# SUMMARY
# ==============================================================================

echo ""
echo "=============================================================================="
echo "UNIX V6 X86 KERNEL - BUILD COMPLETE"
echo "=============================================================================="
echo ""
echo "Project Location: $PROJECT_DIR"
echo ""
echo "Generated Files:"
echo "  • boot.o          - Compiled boot assembly"
echo "  • kernel.o        - Compiled kernel C code"
echo "  • kernel.elf      - Final executable (bootable)"
echo "  • unix_v6.iso     - Bootable ISO image for GRUB"
echo ""
echo "Available Make Targets:"
echo "  • make build      - Compile and link kernel"
echo "  • make iso        - Create bootable ISO"
echo "  • make clean      - Remove build artifacts"
echo "  • make help       - Display help message"
echo ""
echo "Next Steps:"
echo "  1. Boot the ISO on real hardware or with QEMU"
echo "  2. Integrate original V6 source from: /workspaces/unix-v6/source/"
echo "  3. Implement missing subsystems (process, filesystem, drivers)"
echo ""
echo "=============================================================================="
echo ""
