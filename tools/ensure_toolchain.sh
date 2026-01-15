#!/bin/bash
set -e

TARGET="i686-elf-gcc"
PREFIX_BIN="/opt/cross/bin"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Checking for $TARGET...${NC}"

# 1. Check if tool is already in PATH
if command -v $TARGET &> /dev/null; then
    VERSION=$($TARGET --version | head -n1)
    echo -e "${GREEN}✓ $TARGET detected in PATH (${VERSION})${NC}"
    exit 0
fi

# 2. Check standard install location (/opt/cross)
if [ -x "$PREFIX_BIN/$TARGET" ]; then
    echo -e "${YELLOW}! $TARGET found in $PREFIX_BIN but not in PATH${NC}"
    echo -e "${GREEN}Adding $PREFIX_BIN to PATH...${NC}"
    export PATH="$PREFIX_BIN:$PATH"
    
    # Suggest persistent change
    if [[ ":$PATH:" != *":$PREFIX_BIN:"* ]]; then
        echo "Tip: Add this to your ~/.bashrc or ~/.zshrc:"
        echo "  export PATH=\"$PREFIX_BIN:\$PATH\""
    fi
    exit 0
fi

# 3. Not found - Build it
echo -e "${RED}✗ $TARGET not found.${NC}"
echo -e "${YELLOW}Starting build process using local build script...${NC}"

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
BUILD_SCRIPT="$SCRIPT_DIR/build_toolchain.sh"

if [ -f "$BUILD_SCRIPT" ]; then
    chmod +x "$BUILD_SCRIPT"
    "$BUILD_SCRIPT"
else
    echo -e "${RED}Error: Build script not found at $BUILD_SCRIPT${NC}"
    exit 1
fi
