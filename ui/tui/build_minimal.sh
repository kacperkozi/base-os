#!/bin/bash
# Minimal build script for MVP debugging
# Run this to quickly test the minimal version

set -e

echo "Building Minimal MVP Version..."
echo ""

# Clean build
rm -rf build
mkdir build
cd build

# Simple CMake configuration - minimal complexity
# Use vendored FTXUI for ISO deployment compatibility
echo "🔍 Checking for FTXUI in third-party/FTXUI..."

# Check multiple possible paths for FTXUI
FTXUI_PATHS=(
    "../third-party/FTXUI/CMakeLists.txt"
    "../../third-party/FTXUI/CMakeLists.txt"  
    "../ui/third-party/FTXUI/CMakeLists.txt"
)

FTXUI_FOUND=false
for path in "${FTXUI_PATHS[@]}"; do
    if [ -f "$path" ]; then
        FTXUI_FOUND=true
        echo "✅ FTXUI v6.1.9 found at: $path"
        break
    fi
done

if [ "$FTXUI_FOUND" = false ]; then
    echo "❌ FTXUI not found in any expected location"
    echo "📥 Checked paths:"
    for path in "${FTXUI_PATHS[@]}"; do
        echo "   - $path"
    done
    echo ""
    echo "Current directory: $(pwd)"
    echo "Available files:"
    ls -la ../third-party/ 2>/dev/null || echo "   third-party directory not found"
    exit 1
fi
echo "🎯 Building minimal MVP version..."

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=17 \
    -DUSE_DEB_FTXUI=OFF \
    -DUSE_SYSTEM_FTXUI=OFF \
    -DUSE_VENDORED_FTXUI=ON \
    -DUSE_FETCHCONTENT_FTXUI=OFF \
    -DUSE_SYSTEM_ZXING=OFF

echo ""
echo "🔨 Compiling..."

# Build with single thread to avoid complexity
make -j1

if [[ -f "base_os_tui" ]]; then
    echo ""
    echo "✅ Build successful!"
    echo "📍 Executable: $(pwd)/base_os_tui"
    echo "📏 Size: $(du -h base_os_tui | cut -f1)"
    echo ""
    echo "🚀 To run:"
    echo "   ./base_os_tui"
    echo ""
    echo "🎯 MVP Features:"
    echo "   • Simple transaction input"
    echo "   • Basic validation"
    echo "   • Review screen"
    echo "   • No threading (no crashes)"
    echo "   • No complex animations"
    echo "   • No USB scanning"
    echo ""
else
    echo ""
    echo "❌ Build failed"
    echo "Check the output above for errors"
    exit 1
fi
