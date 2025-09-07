#!/bin/bash

# Build script for Base OS TUI with Wallet Detection
# Based on eth-signer-cpp architecture

set -e

echo "🔧 Building Base OS TUI with Wallet Detection"
echo "=============================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found. Please run this script from the tui directory."
    exit 1
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build_wallet_detection
cd build_wallet_detection

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_DEB_FTXUI=OFF \
    -DUSE_SYSTEM_FTXUI=OFF \
    -DUSE_VENDORED_FTXUI=ON \
    -DUSE_FETCHCONTENT_FTXUI=ON \
    -DUSE_SYSTEM_ZXING=OFF

# Build the application
echo "🔨 Building application..."
make -j$(nproc)

# Check if build was successful
if [ -f "base_os_tui" ]; then
    echo "✅ Build successful!"
    echo ""
    echo "🚀 To run the wallet detection TUI:"
    echo "   ./base_os_tui"
    echo ""
    echo "📋 Features:"
    echo "   • Continuous USB device polling"
    echo "   • Ledger device detection"
    echo "   • Real-time status updates"
    echo "   • Based on eth-signer-cpp architecture"
    echo ""
    echo "💡 Make sure your Ledger device is connected and the Ethereum app is open!"
else
    echo "❌ Build failed!"
    exit 1
fi
