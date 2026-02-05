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

install_dependencies() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
    else
        OS=$(uname -s)
    fi

    echo "Detected OS: $OS"

    case "$OS" in
        ubuntu|debian|pop|linuxmint)
            $SUDO apt-get update
            $SUDO apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev curl wget
            ;;
        arch|manjaro)
            echo "Installing dependencies for Arch Linux..."
            $SUDO pacman -Sy --noconfirm
            $SUDO pacman -S --needed --noconfirm base-devel gmp libmpc mpfr isl texinfo bison flex curl wget
            ;;
        fedora)
            echo "Installing dependencies for Fedora..."
            $SUDO dnf install -y @development-tools gmp-devel libmpc-devel mpfr-devel isl-devel texinfo bison flex curl wget
            ;;
        *)
            echo -e "${RED}Unsupported or unknown OS: $OS${NC}"
            echo "Please ensure you have the following installed:"
            echo "  - Build tools (gcc, make, etc.)"
            echo "  - bison, flex, texinfo"
            echo "  - GMP, MPC, MPFR, ISL development libraries"
            echo "  - curl, wget"
            read -p "Press Enter to continue if dependencies are met..."
            ;;
    esac
}

install_dependencies

# 2. Prepare Directories
echo -e "${GREEN}Preparing installation directory at $PREFIX...${NC}"
if [ ! -d "$PREFIX" ]; then
    $SUDO mkdir -p $PREFIX
fi
# Change ownership to current user to avoid building as root
$SUDO chown -R $USER:$USER $PREFIX

# Create a persistent build directory
BUILD_DIR="$(pwd)/toolchain_build"
echo "Working in $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 3. Build Binutils
if [ -f "$PREFIX/bin/$TARGET-ld" ]; then
    echo -e "${GREEN}Binutils already installed. Skipping...${NC}"
else
    echo -e "${GREEN}Checking Binutils source...${NC}"
    echo -e "${GREEN}Downloading/Resuming Binutils $BINUTILS_VERSION...${NC}"
    wget -c https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz

    if [ ! -d "binutils-$BINUTILS_VERSION" ]; then
        echo "Extracting Binutils..."
        tar -xf binutils-$BINUTILS_VERSION.tar.xz
    fi

    mkdir -p build-binutils
    cd build-binutils
    if [ ! -f "Makefile" ]; then
        echo -e "${GREEN}Configuring Binutils...${NC}"
        ../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    fi
    
    echo -e "${GREEN}Compiling Binutils...${NC}"
    make -j$(nproc)
    echo -e "${GREEN}Installing Binutils...${NC}"
    make install
    cd ..
fi

# 4. Build GCC
if [ -f "$PREFIX/bin/$TARGET-gcc" ]; then
     echo -e "${GREEN}GCC already installed. Skipping...${NC}"
else
    echo -e "${GREEN}Checking GCC source...${NC}"
    echo -e "${GREEN}Downloading/Resuming GCC $GCC_VERSION...${NC}"
    wget -c https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz

    if [ ! -d "gcc-$GCC_VERSION" ]; then
        echo "Extracting GCC..."
        tar -xf gcc-$GCC_VERSION.tar.xz
    fi

    # The $PREFIX/bin dir _must_ be in the PATH. We did that above.
    export PATH="$PREFIX/bin:$PATH"

    mkdir -p build-gcc
    cd build-gcc
    if [ ! -f "Makefile" ]; then
        echo -e "${GREEN}Configuring GCC...${NC}"
        ../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    fi

    echo -e "${GREEN}Compiling GCC (this will take a LONG time)...${NC}"
    echo "Running: make -j$(nproc) all-gcc"
    make -j$(nproc) all-gcc
    echo "Running: make -j$(nproc) all-target-libgcc"
    make -j$(nproc) all-target-libgcc
    echo -e "${GREEN}Installing GCC...${NC}"
    make install-gcc
    make install-target-libgcc
    cd ..
fi

# Cleanup
echo -e "${GREEN}Build complete.${NC}"
echo "To save space, you can remove the build directory manually: rm -rf $BUILD_DIR"

echo -e "${GREEN}Success! Toolchain installed to $PREFIX${NC}"
echo "Add this to your PATH: export PATH=\"$PREFIX/bin:\$PATH\""
