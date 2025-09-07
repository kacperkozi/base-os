#!/bin/bash
# Clean up the vendor directory since FTXUI is now in third-party/

echo "🧹 Cleaning up vendor directory..."

if [ -d "vendor" ]; then
    echo "📁 Found vendor directory:"
    ls -la vendor/
    echo ""
    
    if [ -f "../third-party/FTXUI/CMakeLists.txt" ]; then
        echo "✅ FTXUI is properly installed in third-party/FTXUI"
        echo "🗑️ Removing redundant vendor directory..."
        rm -rf vendor/
        echo "✅ Vendor directory cleaned up"
    else
        echo "⚠️ FTXUI not found in third-party, keeping vendor as backup"
    fi
else
    echo "✅ No vendor directory to clean up"
fi

# Also clean up deps directory if it exists
if [ -d "deps" ]; then
    echo "📁 Found deps directory:"
    du -sh deps/
    echo "🗑️ Removing deps directory (created by prepare-dependencies.sh)..."
    rm -rf deps/
    echo "✅ Deps directory cleaned up"
fi

echo ""
echo "🎯 Directory is now clean and ready for minimal build!"
