#!/bin/bash
set -e

mkdir -p build
cd build

cmake .. -DCMAKE_PREFIX_PATH="/usr/local/opt/qt"

cmake --build . -- -j"$(sysctl -n hw.ncpu)"

echo "Build finished!"
