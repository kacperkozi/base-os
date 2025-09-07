#!/bin/bash
# Quick test to verify everything is ready

echo "🔍 Quick Setup Verification..."
echo ""

# Check current directory
echo "📍 Current directory: $(pwd)"
echo ""

# Check FTXUI locations
echo "🔍 Checking FTXUI locations:"
if [ -f "../third-party/FTXUI/CMakeLists.txt" ]; then
    echo "✅ Found: ../third-party/FTXUI/CMakeLists.txt"
else
    echo "❌ Missing: ../third-party/FTXUI/CMakeLists.txt"
fi

if [ -d "../third-party/FTXUI/src" ]; then
    echo "✅ Found: ../third-party/FTXUI/src/"
else
    echo "❌ Missing: ../third-party/FTXUI/src/"
fi

if [ -d "../third-party/FTXUI/include" ]; then
    echo "✅ Found: ../third-party/FTXUI/include/"
else
    echo "❌ Missing: ../third-party/FTXUI/include/"
fi

echo ""

# Check minimal source files
echo "🔍 Checking minimal source files:"
if [ -f "src/hello_world.cpp" ]; then
    echo "✅ Found: src/hello_world.cpp"
else
    echo "❌ Missing: src/hello_world.cpp"
fi

if [ -f "src/main.cpp" ]; then
    echo "✅ Found: src/main.cpp"
else
    echo "❌ Missing: src/main.cpp"
fi

echo ""

# Check CMakeLists.txt configuration
echo "🔍 Checking CMakeLists.txt configuration:"
if grep -q "src/hello_world.cpp" CMakeLists.txt; then
    echo "✅ CMakeLists.txt configured for minimal build"
else
    echo "❌ CMakeLists.txt not configured for minimal build"
fi

echo ""
echo "🚀 If all checks pass, run: ./build_minimal.sh"
