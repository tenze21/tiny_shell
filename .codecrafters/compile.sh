#!/bin/sh
set -e

# Print environment info for debugging
echo "VCPKG_ROOT is: ${VCPKG_ROOT}"
echo "vcpkg location: $(which vcpkg 2>/dev/null || echo 'not in PATH')"
ls /usr/local/vcpkg 2>/dev/null && echo "found at /usr/local/vcpkg" || echo "not at /usr/local/vcpkg"

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build