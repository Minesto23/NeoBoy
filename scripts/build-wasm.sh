#!/bin/bash

# NeoBoy WASM Build Script
# Orchestrates the compilation of C cores to WebAssembly

set -e

echo "======================================"
echo "NeoBoy WASM Build Script"
echo "======================================"

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc (Emscripten) not found in PATH."
    echo "Please source emsdk_env.sh first."
    exit 1
fi

echo "✓ Emscripten found: $(emcc --version | head -n 1)"
echo

# Run Makefile
make all

echo
echo "======================================"
echo "All cores built successfully!"
echo "Generated files in frontend/src/wasm/generated/"
echo "======================================"
