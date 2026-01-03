#!/bin/bash
# DXVK Build Script for TERA PostFX
# Run from MSYS2 MINGW32 terminal

set -e

# Configuration
BUILD_DIR="build.32"
BUILD_TYPE="${1:-release}"  # release or debug
TERA_PATH="/d/games/Tera 92.04 MMOGATE (DEV)/Binaries"

# Ensure glslang is available
export PATH="/ucrt64/bin:$PATH"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}DXVK Build Script${NC}"
echo "Build type: $BUILD_TYPE"
echo ""

# Check if we need to configure
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Configuring build...${NC}"
    meson setup --buildtype "$BUILD_TYPE" "$BUILD_DIR"
fi

# Build
echo -e "${YELLOW}Building...${NC}"
ninja -C "$BUILD_DIR"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
    echo "Output: $BUILD_DIR/src/d3d9/d3d9.dll"

    # Ask to deploy
    echo ""
    read -p "Deploy to TERA? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cp "$BUILD_DIR/src/d3d9/d3d9.dll" "$TERA_PATH/"
        echo -e "${GREEN}Deployed to TERA!${NC}"
    fi
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
