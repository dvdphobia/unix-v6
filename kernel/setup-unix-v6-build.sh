#!/bin/bash
# setup-unix-v6-build.sh - Unix V6 x86 Kernel Build Environment Setup
# This script installs all required cross-compilation tools on Ubuntu/Debian

set -e  # Exit on error

echo "========================================================================"
echo "Unix V6 x86 Kernel - Build Environment Setup"
echo "========================================================================"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running on Ubuntu/Debian
if ! [ -f /etc/debian_version ]; then
    echo -e "${RED}Error: This script is designed for Ubuntu/Debian systems${NC}"
    echo "Detected OS: $(uname -s)"
    exit 1
fi

echo -e "${YELLOW}Step 1: Update package lists${NC}"
sudo apt-get update

echo ""
echo -e "${YELLOW}Step 2: Install build essentials${NC}"
sudo apt-get install -y \
    build-essential \
    wget \
    curl \
    git \
    bison \
    flex \
    texinfo

echo ""
echo -e "${YELLOW}Step 3: Install cross-compiler toolchain (i686-elf-gcc)${NC}"
sudo apt-get install -y \
    gcc-multilib \
    g++-multilib

echo ""
echo -e "${YELLOW}Step 4: Install binutils and GDB for i686-elf${NC}"
sudo apt-get install -y \
    binutils

echo ""
echo -e "${YELLOW}Step 5: Install GRUB utilities for ISO generation${NC}"
sudo apt-get install -y \
    grub-common \
    grub-pc \
    grub-pc-bin \
    xorriso

echo ""
echo -e "${YELLOW}Step 6: Installing i686-elf cross-compiler toolchain${NC}"

# Create temporary build directory
BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT

echo "Building in: $BUILD_DIR"

# Download and build binutils
cd "$BUILD_DIR"
BINUTILS_VERSION="2.39"
echo "Downloading binutils-${BINUTILS_VERSION}..."
wget -q https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz || \
    echo -e "${RED}Warning: Could not download binutils, checking if i686-elf-gcc is available${NC}"

if [ -f binutils-${BINUTILS_VERSION}.tar.gz ]; then
    tar xzf binutils-${BINUTILS_VERSION}.tar.gz
    cd binutils-${BINUTILS_VERSION}
    ./configure --target=i686-elf --prefix=/opt/cross --disable-nls --disable-werror
    make -j$(nproc)
    sudo make install
    cd "$BUILD_DIR"
fi

# Check if i686-elf-gcc is available from repositories
if ! command -v i686-elf-gcc &> /dev/null; then
    echo -e "${YELLOW}Step 7: Building GCC cross-compiler (this may take a while)${NC}"
    
    GCC_VERSION="12.2.0"
    echo "Downloading gcc-${GCC_VERSION}..."
    wget -q https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz || \
        echo -e "${RED}Warning: Could not download GCC${NC}"
    
    if [ -f gcc-${GCC_VERSION}.tar.gz ]; then
        tar xzf gcc-${GCC_VERSION}.tar.gz
        cd gcc-${GCC_VERSION}
        ./contrib/download_prerequisites
        mkdir build
        cd build
        ../configure --target=i686-elf --prefix=/opt/cross \
            --disable-nls --enable-languages=c --without-headers --disable-werror
        make -j$(nproc) all-gcc
        sudo make install-gcc
    fi
fi

echo ""
echo -e "${GREEN}========================================================================"
echo "Installation Complete!"
echo "========================================================================${NC}"
echo ""

# Add cross-compiler to PATH if not already there
if ! grep -q "/opt/cross/bin" ~/.bashrc; then
    echo 'export PATH="/opt/cross/bin:$PATH"' >> ~/.bashrc
    echo -e "${GREEN}Added /opt/cross/bin to PATH in ~/.bashrc${NC}"
fi
export PATH="/opt/cross/bin:$PATH"

# Verify installation
echo -e "${YELLOW}Verifying toolchain installation...${NC}"
echo ""

TOOLS=(
    "i686-elf-gcc"
    "i686-elf-as"
    "i686-elf-ld"
    "i686-elf-objcopy"
    "grub-mkrescue"
)

ALL_FOUND=true
for tool in "${TOOLS[@]}"; do
    if command -v "$tool" &> /dev/null; then
        VERSION=$($tool --version 2>/dev/null | head -n1)
        echo -e "${GREEN}✓${NC} $tool: $VERSION"
    else
        echo -e "${RED}✗${NC} $tool: NOT FOUND"
        ALL_FOUND=false
    fi
done

echo ""
if [ "$ALL_FOUND" = true ]; then
    echo -e "${GREEN}All required tools are installed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. cd /workspaces/unix-v6/kernel"
    echo "2. make clean"
    echo "3. make build"
    echo "4. make iso"
    echo ""
    echo "Your bootable ISO will be created as: unix_v6.iso"
else
    echo -e "${RED}Some tools are missing. Please check the errors above.${NC}"
    exit 1
fi
