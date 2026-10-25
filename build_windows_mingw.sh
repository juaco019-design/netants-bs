#!/bin/bash
# build_windows_mingw.sh - Script rapido para compilar en MSYS2 MinGW64
set -e
BUILD_TYPE=${1:-Release}
echo "=== NetAnts Reborn Build ($BUILD_TYPE) GCC Windows ==="
mkdir -p build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
mingw32-make -j$(nproc)
echo "Binario: $(pwd)/NetAntsReborn.exe"
ls -lh NetAntsReborn.exe
