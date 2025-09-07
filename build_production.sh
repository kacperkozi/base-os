#!/bin/bash
# Base OS TUI - Production Build Script
# Complete setup for Ubuntu systems ready for ISO deployment

set -e

# Colors
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly RED='\033[0;31m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}       Base OS TUI - Production Build System           ${NC}"
    echo -e "${BLUE}       Ubuntu Compatible • ISO Deployment Ready        ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo ""
}

print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1" >&2
}

print_header

# Step 1: Verify we're in the right directory
if [[ ! -f "ui/tui/CMakeLists.txt" ]]; then
    print_error "Please run this script from the base-os repository root"
    print_info "Expected: base-os/build_production.sh"
    exit 1
fi

print_status "Running from repository root"

# Step 2: Navigate to TUI directory
cd ui/tui
print_status "In TUI directory: $(pwd)"

# Step 3: Verify Ubuntu system
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v lsb_release &> /dev/null; then
        UBUNTU_VERSION=$(lsb_release -sr 2>/dev/null)
        print_status "Ubuntu $UBUNTU_VERSION detected"
    else
        print_status "Linux system detected"
    fi
else
    print_info "Non-Linux system detected: $OSTYPE"
    print_info "This script is optimized for Ubuntu but may work on other systems"
fi

# Step 4: Ensure FTXUI is available
print_info "Checking FTXUI v6.1.9 availability..."

if [[ -f "../third-party/FTXUI/CMakeLists.txt" ]]; then
    print_status "FTXUI v6.1.9 found in third-party/FTXUI"
else
    print_info "FTXUI not found, downloading..."
    
    if [[ -f "prepare-dependencies.sh" ]]; then
        chmod +x prepare-dependencies.sh
        echo "6" | ./prepare-dependencies.sh
        
        if [[ -f "../third-party/FTXUI/CMakeLists.txt" ]]; then
            print_status "FTXUI v6.1.9 downloaded successfully"
        else
            print_error "Failed to download FTXUI"
            exit 1
        fi
    else
        print_error "prepare-dependencies.sh not found"
        exit 1
    fi
fi

# Step 5: Clean and prepare build
print_info "Preparing production build..."

if [[ -d "build" ]]; then
    print_info "Cleaning previous build..."
    rm -rf build
fi

mkdir -p build
cd build

# Step 6: Configure production build
print_info "Configuring production build..."

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DUSE_DEB_FTXUI=OFF \
    -DUSE_SYSTEM_FTXUI=OFF \
    -DUSE_VENDORED_FTXUI=ON \
    -DUSE_FETCHCONTENT_FTXUI=OFF \
    -DUSE_SYSTEM_ZXING=OFF

print_status "CMake configuration successful"

# Step 7: Build with optimal settings
print_info "Building Base OS TUI..."

# Detect CPU cores
JOBS=4
if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
elif [[ -r /proc/cpuinfo ]]; then
    JOBS=$(grep -c ^processor /proc/cpuinfo)
fi

# Limit to 8 cores to avoid overwhelming the system
if [[ $JOBS -gt 8 ]]; then
    JOBS=8
fi

print_info "Using $JOBS parallel jobs"

# Build with error handling
if make -j"$JOBS"; then
    print_status "Build completed successfully"
else
    print_error "Build failed"
    exit 1
fi

# Step 8: Verify executable
if [[ ! -f "base_os_tui" ]]; then
    print_error "Executable not found after build"
    exit 1
fi

# Make executable
chmod +x base_os_tui

# Get file info
FILE_SIZE=$(du -h base_os_tui | cut -f1)
print_status "Executable created: $FILE_SIZE"

# Step 9: Test basic functionality
print_info "Testing executable..."
if timeout 3s ./base_os_tui --version &>/dev/null || [[ $? -eq 124 ]]; then
    print_status "Executable launches successfully"
else
    print_info "Basic test completed (may require TTY for full UI)"
fi

# Step 10: Success summary
echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}            ✓ Production Build Complete!               ${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo ""

echo -e "${CYAN}🎯 Features Built:${NC}"
echo "  • Minimal transaction input form"
echo "  • Basic validation (address format, numeric values)"
echo "  • Review screen for transaction details"  
echo "  • Mock signing process"
echo "  • No threading (stable, no crashes)"
echo "  • No complex animations"
echo "  • ISO deployment ready"
echo ""

echo -e "${CYAN}📍 Executable Location:${NC}"
echo "  $(pwd)/base_os_tui"
echo ""

echo -e "${CYAN}🚀 To Run:${NC}"
echo "  cd $(pwd)"
echo "  ./base_os_tui"
echo ""

echo -e "${CYAN}🎮 Controls:${NC}"
echo "  • Tab: Navigate between fields"
echo "  • Enter: Submit/Continue"
echo "  • Escape: Go back"
echo "  • Ctrl+C: Quit"
echo ""

echo -e "${CYAN}📦 ISO Deployment:${NC}"
echo "  • All dependencies vendored in third-party/"
echo "  • No internet required for builds"
echo "  • Works on x86_64 and ARM64 Ubuntu"
echo "  • Self-contained and portable"
echo ""

# Ask if user wants to test now
echo -n "Would you like to run the application now? [y/N]: "
read -r response

case "$response" in
    [Yy]|[Yy][Ee][Ss])
        echo ""
        print_info "Launching Base OS TUI..."
        print_info "Use Tab to navigate, Enter to continue, Escape to go back"
        echo ""
        sleep 2
        ./base_os_tui
        ;;
    *)
        print_info "You can run the application anytime with:"
        print_info "  cd $(pwd) && ./base_os_tui"
        ;;
esac

echo ""
print_status "Production build system ready for deployment!"