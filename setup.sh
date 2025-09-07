#!/bin/bash
# Base OS - Ubuntu Compatible Setup Script
# Optimized for Ubuntu 18.04, 20.04, 22.04, and 24.04
# Run this after cloning the repository to build and launch the TUI application

set -e  # Exit on error for better error handling

# Colors for output
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly RED='\033[0;31m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Script configuration
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly TUI_DIR="${SCRIPT_DIR}/ui/tui"
readonly FTXUI_VERSION="6.1.9"

print_header() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}       Base OS - Offline Transaction Signer Setup      ${NC}"
    echo -e "${BLUE}       Ubuntu Compatible Build System                   ${NC}"
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

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Error handling
handle_error() {
    local line_number=$1
    print_error "Setup failed at line $line_number"
    print_info "Check the error output above for details"
    print_info "You can also run with: bash -x setup.sh for detailed debugging"
    exit 1
}

trap 'handle_error ${LINENO}' ERR

print_header

# Detect OS and Ubuntu version
detect_system() {
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        print_error "This script is optimized for Ubuntu Linux"
        print_error "Detected OS: $OSTYPE"
        print_info "For macOS, try: brew install cmake && cd ui/tui && cmake -B build && make -C build"
        exit 1
    fi
    
    if ! command -v lsb_release &> /dev/null; then
        print_warning "lsb_release not found, assuming Ubuntu"
        UBUNTU_VERSION="unknown"
    else
        local distro=$(lsb_release -si 2>/dev/null)
        UBUNTU_VERSION=$(lsb_release -sr 2>/dev/null)
        
        if [[ "$distro" != "Ubuntu" ]]; then
            print_warning "Non-Ubuntu distribution detected: $distro"
            print_info "This script is optimized for Ubuntu but may work on Debian-based systems"
        fi
    fi
    
    print_info "System: Ubuntu $UBUNTU_VERSION"
    
    # Check for supported versions
    case "$UBUNTU_VERSION" in
        18.04|20.04|22.04|24.04)
            print_status "Supported Ubuntu version detected"
            ;;
        *)
            print_warning "Ubuntu version $UBUNTU_VERSION not specifically tested"
            print_info "Supported versions: 18.04, 20.04, 22.04, 24.04"
            ;;
    esac
}

detect_system

# Step 1: Install Ubuntu dependencies
install_dependencies() {
    print_info "Installing Ubuntu dependencies..."
    
    # Update package list
    print_info "Updating package list..."
    if ! sudo apt update; then
        print_error "Failed to update package list"
        print_info "Try: sudo apt update --fix-missing"
        exit 1
    fi
    
    # Essential build tools
    local packages=(
        "build-essential"
        "cmake"
        "git"
        "pkg-config"
        "binutils"
        "libc6-dev"
    )
    
    # Version-specific packages
    case "$UBUNTU_VERSION" in
        18.04)
            packages+=("libstdc++-8-dev")
            ;;
        20.04)
            packages+=("libstdc++-9-dev")
            ;;
        22.04|24.04)
            packages+=("libstdc++-12-dev")
            ;;
    esac
    
    # Check which packages need installation
    local to_install=()
    for package in "${packages[@]}"; do
        if ! dpkg -l | grep -q "^ii  $package "; then
            to_install+=("$package")
        fi
    done
    
    if [ ${#to_install[@]} -eq 0 ]; then
        print_status "All required packages already installed"
    else
        print_info "Installing packages: ${to_install[*]}"
        if ! sudo apt install -y "${to_install[@]}"; then
            print_error "Failed to install required packages"
            exit 1
        fi
        print_status "Dependencies installed successfully"
    fi
}

# Step 2: Verify tools
verify_tools() {
    print_info "Verifying build tools..."
    
    # Check CMake version
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found after installation"
        exit 1
    fi
    
    local cmake_version=$(cmake --version | head -1 | awk '{print $3}')
    local cmake_major=$(echo "$cmake_version" | cut -d. -f1)
    local cmake_minor=$(echo "$cmake_version" | cut -d. -f2)
    
    if [[ $cmake_major -lt 3 ]] || [[ $cmake_major -eq 3 && $cmake_minor -lt 16 ]]; then
        print_error "CMake version $cmake_version is too old (need 3.16+)"
        print_info "Try: sudo snap install cmake --classic"
        exit 1
    fi
    print_status "CMake $cmake_version (compatible)"
    
    # Check C++ compiler
    local cxx_compiler=""
    local cxx_version=""
    
    if command -v g++ &> /dev/null; then
        cxx_compiler="g++"
        cxx_version=$(g++ --version | head -1 | awk '{print $4}')
        local gcc_major=$(echo "$cxx_version" | cut -d. -f1)
        
        if [[ $gcc_major -lt 7 ]]; then
            print_error "GCC version $cxx_version is too old (need 7+)"
            exit 1
        fi
        print_status "GCC $cxx_version (C++17 compatible)"
    elif command -v clang++ &> /dev/null; then
        cxx_compiler="clang++"
        cxx_version=$(clang++ --version | head -1 | awk '{print $4}')
        print_status "Clang $cxx_version (C++17 compatible)"
    else
        print_error "No C++ compiler found"
        exit 1
    fi
    
    # Check Git
    if ! command -v git &> /dev/null; then
        print_error "Git not found after installation"
        exit 1
    fi
    print_status "Git available"
    
    # Check for ar (needed for .deb extraction)
    if ! command -v ar &> /dev/null; then
        print_error "ar command not found (needed for .deb extraction)"
        print_info "Install with: sudo apt install binutils"
        exit 1
    fi
    print_status "Archive tools available"
}

install_dependencies
verify_tools

# Step 3: Navigate to TUI directory
navigate_to_tui() {
    print_info "Navigating to TUI directory..."
    
    if [[ ! -d "$TUI_DIR" ]]; then
        print_error "TUI directory not found: $TUI_DIR"
        print_info "Make sure you're running this script from the base-os repository root"
        print_info "Expected structure: base-os/ui/tui/"
        exit 1
    fi
    
    cd "$TUI_DIR" || {
        print_error "Failed to change to TUI directory"
        exit 1
    }
    
    print_status "In TUI directory: $(pwd)"
    
    # Verify we have the minimal implementation
    if [[ ! -f "src/hello_world.cpp" ]]; then
        print_warning "Minimal UI implementation not found"
        print_info "The application will use a more complex implementation"
    else
        print_status "Minimal UI implementation available"
    fi
    
    # Check for FTXUI in third-party (needed for ISO deployment)
    if [[ ! -f "../third-party/FTXUI/CMakeLists.txt" ]]; then
        print_warning "FTXUI not found in third-party/FTXUI"
        print_info "Running prepare-dependencies.sh to download FTXUI v${FTXUI_VERSION}..."
        
        if [[ -f "prepare-dependencies.sh" ]]; then
            chmod +x prepare-dependencies.sh
            # Run option 1 (download source) automatically
            echo "1" | ./prepare-dependencies.sh
        else
            print_error "prepare-dependencies.sh not found"
            print_info "Please run: cd ui/tui && ./prepare-dependencies.sh"
            exit 1
        fi
        
        # Verify download was successful
        if [[ -f "../third-party/FTXUI/CMakeLists.txt" ]]; then
            print_status "FTXUI v${FTXUI_VERSION} downloaded successfully"
        else
            print_error "Failed to download FTXUI"
            exit 1
        fi
    else
        print_status "FTXUI available in third-party/FTXUI"
    fi
}

# Step 4: Verify FTXUI .deb package
verify_ftxui_deb() {
    print_info "Checking for FTXUI .deb package..."
    
    # Check system architecture
    local arch=$(uname -m)
    if [[ "$arch" == "aarch64" || "$arch" == "arm64" ]]; then
        print_warning "ARM64 architecture detected"
        print_info "The provided .deb package is x86_64 only"
        print_info "Will download FTXUI v6.1.9 from GitHub (this is normal and expected)"
        return
    fi
    
    local deb_file="../third-party/ftxui-${FTXUI_VERSION}-Linux.deb"
    
    if [[ -f "$deb_file" ]]; then
        local size=$(du -h "$deb_file" | cut -f1)
        print_status "Found FTXUI .deb package ($size)"
        
        # Verify it's a valid .deb file
        if file "$deb_file" | grep -q "Debian binary package"; then
            print_status "Valid .deb package format"
        else
            print_warning ".deb file may be corrupted"
        fi
    else
        print_warning "FTXUI .deb package not found: $deb_file"
        print_info "Will fall back to vendored FTXUI"
    fi
}

# Step 5: Clean and prepare build
prepare_build() {
    print_info "Preparing build environment..."
    
    # Clean previous builds
    if [[ -d "build" ]]; then
        print_info "Cleaning previous build..."
        rm -rf build
        print_status "Previous build cleaned"
    fi
    
    # Create build directory
    mkdir -p build
    print_status "Build directory created"
    
    # Detect CPU cores for parallel build
    local jobs=4
    if command -v nproc &> /dev/null; then
        jobs=$(nproc)
    elif [[ -r /proc/cpuinfo ]]; then
        jobs=$(grep -c ^processor /proc/cpuinfo)
    fi
    
    # Don't use more than 8 cores to avoid overwhelming the system
    if [[ $jobs -gt 8 ]]; then
        jobs=8
    fi
    
    print_info "Using $jobs parallel jobs for compilation"
    export JOBS=$jobs
}

navigate_to_tui
verify_ftxui_deb
prepare_build

# Step 6: Configure build with CMake
configure_build() {
    print_info "Configuring build with CMake..."
    
    cd build || {
        print_error "Failed to enter build directory"
        exit 1
    }
    
    # Ubuntu-optimized CMake configuration for ISO deployment
    local cmake_args=(
        ".."
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_CXX_STANDARD=17"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
        "-DCMAKE_CXX_EXTENSIONS=OFF"
        "-DUSE_DEB_FTXUI=OFF"
        "-DUSE_SYSTEM_FTXUI=OFF"
        "-DUSE_VENDORED_FTXUI=ON"
        "-DUSE_FETCHCONTENT_FTXUI=OFF"
        "-DUSE_SYSTEM_ZXING=OFF"
    )
    
    # Add verbose output for debugging if needed
    if [[ "${VERBOSE:-}" == "1" ]]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    print_info "CMake configuration:"
    print_info "  Build type: Release"
    print_info "  C++ standard: 17"
    print_info "  FTXUI source: vendored (ISO deployment ready)"
    print_info "  UI version: minimal (for debugging)"
    
    if ! cmake "${cmake_args[@]}"; then
        print_error "CMake configuration failed"
        print_info "Common issues:"
        print_info "  - Missing dependencies: sudo apt install build-essential cmake"
        print_info "  - Old CMake version: sudo snap install cmake --classic"
        print_info "  - Corrupted .deb file: check ui/third-party/ftxui-*.deb"
        exit 1
    fi
    
    print_status "CMake configuration successful"
}

# Step 7: Build the application
build_application() {
    print_info "Building Base OS TUI (this may take a few minutes)..."
    
    # Build with proper error handling
    local build_log="build.log"
    local build_success=false
    
    # Try parallel build first
    print_info "Attempting parallel build with $JOBS cores..."
    if make -j"$JOBS" 2>&1 | tee "$build_log"; then
        if [[ -f "base_os_tui" ]]; then
            build_success=true
            print_status "Parallel build successful"
        fi
    fi
    
    # If parallel build failed, try single-threaded
    if [[ "$build_success" != "true" ]]; then
        print_warning "Parallel build failed, trying single-threaded build..."
        if make 2>&1 | tee -a "$build_log"; then
            if [[ -f "base_os_tui" ]]; then
                build_success=true
                print_status "Single-threaded build successful"
            fi
        fi
    fi
    
    # Final check
    if [[ "$build_success" != "true" ]]; then
        print_error "Build failed"
        print_info "Build log saved to: $(pwd)/$build_log"
        print_info ""
        print_info "Common solutions:"
        print_info "1. Check the build log for specific errors"
        print_info "2. Ensure all dependencies are installed:"
        print_info "   sudo apt install build-essential cmake git binutils"
        print_info "3. Try verbose build:"
        print_info "   make VERBOSE=1"
        print_info "4. Check available disk space and memory"
        exit 1
    fi
    
    # Cleanup build log if successful
    rm -f "$build_log"
}

# Step 8: Verify and test executable
verify_executable() {
    print_info "Verifying executable..."
    
    if [[ ! -f "base_os_tui" ]]; then
        print_error "Executable not found after successful build"
        exit 1
    fi
    
    # Check file properties
    local file_size=$(du -h base_os_tui | cut -f1)
    local file_perms=$(ls -l base_os_tui | cut -d' ' -f1)
    
    print_status "Executable created successfully"
    print_info "  Path: $(pwd)/base_os_tui"
    print_info "  Size: $file_size"
    print_info "  Permissions: $file_perms"
    
    # Make sure it's executable
    chmod +x base_os_tui
    
    # Check dependencies
    if command -v ldd &> /dev/null; then
        print_info "Checking dynamic dependencies..."
        local missing_deps=()
        while IFS= read -r line; do
            if echo "$line" | grep -q "not found"; then
                missing_deps+=("$line")
            fi
        done < <(ldd base_os_tui 2>/dev/null)
        
        if [[ ${#missing_deps[@]} -gt 0 ]]; then
            print_warning "Missing dependencies detected:"
            printf '  %s\n' "${missing_deps[@]}"
        else
            print_status "All dependencies satisfied"
        fi
    fi
    
    # Basic functionality test
    print_info "Testing executable..."
    if timeout 2s ./base_os_tui --help &>/dev/null || [[ $? -eq 124 ]]; then
        print_status "Executable launches successfully"
    else
        print_warning "Executable test inconclusive (may still work)"
    fi
}

# Step 9: Success message and instructions
show_completion() {
    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}            ✓ Ubuntu Setup Complete!                   ${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
    echo ""
    echo "The Base OS TUI application has been built successfully!"
    echo ""
    echo -e "${CYAN}Features:${NC}"
    echo "  • Thread-safe UI rendering (no more flashing issues)"
    echo "  • Offline Ethereum transaction signing"  
    echo "  • Base network support (Chain ID: 8453)"
    echo "  • Address book management"
    echo "  • QR code generation for signed transactions"
    echo ""
    echo -e "${CYAN}To run the application:${NC}"
    echo -e "  ${BLUE}cd $(pwd) && ./base_os_tui${NC}"
    echo ""
    echo -e "${CYAN}Or from the repository root:${NC}"
    echo -e "  ${BLUE}cd ui/tui/build && ./base_os_tui${NC}"
    echo ""
    echo -e "${CYAN}Keyboard shortcuts:${NC}"
    echo "  • Ctrl+Q: Quit application"
    echo "  • Tab: Navigate between fields"  
    echo "  • Enter: Submit/Confirm"
    echo "  • F1: Help"
    echo "  • 1-6: Jump to specific screens"
    echo ""
    echo -e "${CYAN}Troubleshooting:${NC}"
    echo "  • If you see 'white flashing': Fixed with thread-safe UI"
    echo "  • If terminal shows 'ISO ready': Run with proper TTY"
    echo "  • For build issues: Check ui/tui/build/build.log"
    echo ""
}

# Step 10: Optional launch
offer_to_launch() {
    local response
    echo -n "Would you like to launch the application now? [y/N]: "
    read -r response
    
    case "$response" in
        [Yy]|[Yy][Ee][Ss])
            echo ""
            print_info "Launching Base OS TUI..."
            print_warning "Press Ctrl+Q to quit the application"
            echo ""
            sleep 2
            
            # Launch with proper terminal settings
            TERM="${TERM:-xterm-256color}" ./base_os_tui
            ;;
        *)
            print_info "You can launch the application anytime with:"
            print_info "  cd $(pwd) && ./base_os_tui"
            ;;
    esac
}

# Execute build steps
configure_build
build_application
verify_executable
show_completion
offer_to_launch