#!/bin/bash
set -e

for dir in client server; do
	echo "Building $dir..."
	rm -rf "$dir/build"
	cmake -S "$dir" -B "$dir/build" -DCMAKE_PREFIX_PATH="/usr/local/opt/qt"
	cmake --build "$dir/build" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
done

echo "Build finished!"
