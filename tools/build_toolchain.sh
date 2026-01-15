#!/bin/bash
set -e

# Configuration
PREFIX="/opt/cross"
TARGET="i686-elf"
BINUTILS_VERSION="2.41"
GCC_VERSION="13.2.0"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${GREEN}Starting cross-compiler build script...${NC}"

# Check for sudo
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}Please run as root or with sudo to install dependencies and create directories.${NC}"
  echo "Trying to use sudo for setup steps..."
  SUDO="sudo"
else
  SUDO=""
fi

# 1. Install Dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
$SUDO apt-get update
$SUDO apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev curl wget

# 2. Prepare Directories
echo -e "${GREEN}Preparing installation directory at $PREFIX...${NC}"
if [ ! -d "$PREFIX" ]; then
    $SUDO mkdir -p $PREFIX
fi
# Change ownership to current user to avoid building as root
$SUDO chown -R $USER:$USER $PREFIX

# Create a temporary build directory
BUILD_DIR=$(mktemp -d)
echo "Working in $BUILD_DIR"
cd $BUILD_DIR

# 3. Build Binutils
echo -e "${GREEN}Downloading Binutils $BINUTILS_VERSION...${NC}"
wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
tar -xf binutils-$BINUTILS_VERSION.tar.xz

mkdir build-binutils
cd build-binutils
echo -e "${GREEN}Configuring Binutils...${NC}"
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
echo -e "${GREEN}Compiling Binutils (this may take a while)...${NC}"
make -j$(nproc)
echo -e "${GREEN}Installing Binutils...${NC}"
make install
cd ..

# 4. Build GCC
echo -e "${GREEN}Downloading GCC $GCC_VERSION...${NC}"
wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz
tar -xf gcc-$GCC_VERSION.tar.xz

# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
export PATH="$PREFIX/bin:$PATH"

mkdir build-gcc
cd build-gcc
echo -e "${GREEN}Configuring GCC...${NC}"
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
echo -e "${GREEN}Compiling GCC (this will take a LONG time)...${NC}"
echo "Running: make -j$(nproc) all-gcc"
make -j$(nproc) all-gcc
echo "Running: make -j$(nproc) all-target-libgcc"
make -j$(nproc) all-target-libgcc
echo -e "${GREEN}Installing GCC...${NC}"
make install-gcc
make install-target-libgcc
cd ..

# Cleanup
echo -e "${GREEN}Cleaning up build files...${NC}"
rm -rf $BUILD_DIR

echo -e "${GREEN}Success! Toolchain installed to $PREFIX${NC}"
echo "Add this to your PATH: export PATH=\"$PREFIX/bin:\$PATH\""
