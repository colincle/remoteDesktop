#!/bin/bash
set -e

# Set Qt in PATH
export PATH=/usr/lib/qt6/bin:$PATH

# Build Client
echo "Building Client..."
rm -rf client/build
cd client
./build.sh
cd ..

# Build Server
echo "Building Server..."
rm -rf server/build
cd server
./build.sh
cd ..

# Deploy Client AppImage (ignore errors)
echo "Deploying Client AppImage..."
../linuxdeployqt/build/tools/linuxdeployqt/linuxdeployqt client/build/Client -appimage || true

# Deploy Server AppImage (ignore errors)
echo "Deploying Server AppImage..."
../linuxdeployqt/build/tools/linuxdeployqt/linuxdeployqt server/build/Server -appimage || true

echo "Done."

