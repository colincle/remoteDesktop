#!/bin/bash
set -e

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . -- -j$(nproc)

echo "Build finished!"
