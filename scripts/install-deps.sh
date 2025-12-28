#!/bin/bash
# NeoBoy - Dependency Installation Script
# Installs Emscripten and Node.js dependencies

set -e

echo "======================================"
echo "NeoBoy - Dependency Installation"
echo "======================================"
echo ""

# Check Node.js
if ! command -v node &> /dev/null; then
    echo "Error: Node.js not found!"
    echo "Please install Node.js: https://nodejs.org/"
    exit 1
fi

echo "✓ Node.js found: $(node --version)"

# Check npm
if ! command -v npm &> /dev/null; then
    echo "Error: npm not found!"
    exit 1
fi

echo "✓ npm found: $(npm --version)"
echo ""

# Install frontend dependencies
echo "Installing frontend dependencies..."
cd "$(dirname "$0")/../frontend"
npm install
echo "✓ Frontend dependencies installed"
echo ""

# Check Emscripten
cd ..
if ! command -v emcc &> /dev/null; then
    echo "⚠ Emscripten not found!"
    echo ""
    echo "To install Emscripten:"
    echo "1. Clone the emsdk repository:"
    echo "   git clone https://github.com/emscripten-core/emsdk.git"
    echo "2. Navigate to emsdk directory and run:"
    echo "   ./emsdk install latest"
    echo "   ./emsdk activate latest"
    echo "   source ./emsdk_env.sh"
    echo ""
    echo "For more information: https://emscripten.org/docs/getting_started/downloads.html"
else
    echo "✓ Emscripten found: $(emcc --version | head -n 1)"
fi

echo ""
echo "======================================"
echo "Dependency check complete!"
echo "======================================"
