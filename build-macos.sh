#!/usr/bin/env bash
set -e

cmake -S . -B build-macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64"

cmake --build build-macos -j

echo
echo "Build finished."
echo "App bundle: build-macos/Backrooms Offical.app"
