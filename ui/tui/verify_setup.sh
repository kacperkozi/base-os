#!/bin/bash
# Verify setup is ready for minimal MVP build

echo "🔍 Verifying Base OS TUI Setup..."
echo ""

# Check FTXUI
if [ -f "../third-party/FTXUI/CMakeLists.txt" ]; then
    echo "✅ FTXUI v6.1.9 found in third-party/FTXUI"
else
    echo "❌ FTXUI not found"
    echo "   Run: ./prepare-dependencies.sh (choose option 1)"
    exit 1
fi

# Check minimal implementation
if [ -f "src/hello_world.cpp" ]; then
    echo "✅ Minimal hello world implementation available"
else
    echo "❌ hello_world.cpp not found"
    exit 1
fi

# Check build script
if [ -f "build_minimal.sh" ]; then
    echo "✅ Minimal build script available"
else
    echo "❌ build_minimal.sh not found"
    exit 1
fi

# Check CMakeLists.txt has minimal configuration
if grep -q "src/hello_world.cpp" CMakeLists.txt; then
    echo "✅ CMakeLists.txt configured for minimal build"
else
    echo "❌ CMakeLists.txt not configured for minimal build"
    exit 1
fi

echo ""
echo "🎉 Setup verification complete!"
echo ""
echo "Ready to build minimal MVP:"
echo "  ./build_minimal.sh"
echo "  ./build/base_os_tui"
