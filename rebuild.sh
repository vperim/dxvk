#!/bin/bash
# Clean rebuild script
# Run from MSYS2 MINGW32 terminal

set -e

BUILD_DIR="build.32"
BUILD_TYPE="${1:-release}"

export PATH="/ucrt64/bin:$PATH"

echo "Cleaning build directory..."
rm -rf "$BUILD_DIR"

echo "Configuring..."
meson setup --buildtype "$BUILD_TYPE" "$BUILD_DIR"

echo "Building..."
ninja -C "$BUILD_DIR"

echo "Done! Output: $BUILD_DIR/src/d3d9/d3d9.dll"
