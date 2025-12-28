#!/bin/bash
# NeoBoy - Development Server Launcher
# Starts the React development server

set -e

echo "======================================"
echo "NeoBoy Development Server"
echo "======================================"
echo ""

# Check if in frontend directory
cd "$(dirname "$0")/../frontend"

# Check if node_modules exists
if [ ! -d "node_modules" ]; then
    echo "Installing dependencies..."
    npm install
    echo "✓ Dependencies installed"
    echo ""
fi

echo "Starting development server..."
echo "Frontend will be available at: http://localhost:3000"
echo ""

npm run dev
