#!/usr/bin/env bash

# Base OS TUI Build Script
# Simple build script with automatic dependency fetching

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
USE_FETCHCONTENT_FTXUI="${USE_FETCHCONTENT_FTXUI:-OFF}"

# Functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}   Base OS TUI - Build Script${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found! Please install CMake 3.18 or later."
        exit 1
    fi
    
    # Check C++ compiler
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "C++ compiler not found! Please install g++ or clang++."
        exit 1
    fi
    
    # Check Git (for fetching dependencies)
    if ! command -v git &> /dev/null; then
        print_warning "Git not found! Cannot fetch dependencies automatically."
        USE_LOCAL_FTXUI="ON"
    fi
    
    print_success "All required dependencies found!"
}

setup_build_directory() {
    print_info "Setting up build directory..."
    mkdir -p "$BUILD_DIR"
}

configure_cmake() {
    print_info "Configuring CMake (Build type: $BUILD_TYPE)..."
    
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DUSE_FETCHCONTENT_FTXUI="$USE_FETCHCONTENT_FTXUI"
    
    if [ $? -eq 0 ]; then
        print_success "CMake configuration complete!"
    else
        print_error "CMake configuration failed!"
        exit 1
    fi
    
    cd ..
}

build_project() {
    print_info "Building project with $JOBS parallel jobs..."
    
    cd "$BUILD_DIR"
    cmake --build . -j"$JOBS"
    
    if [ $? -eq 0 ]; then
        print_success "Build complete!"
        echo
        print_success "Executable: $BUILD_DIR/base_os_tui"
    else
        print_error "Build failed!"
        exit 1
    fi
    
    cd ..
}

run_application() {
    if [ -f "$BUILD_DIR/base_os_tui" ]; then
        print_info "Running Base OS TUI..."
        echo -e "${YELLOW}========================================${NC}"
        "$BUILD_DIR/base_os_tui"
    else
        print_error "Executable not found! Build the project first."
        exit 1
    fi
}

clean_build() {
    print_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    print_success "Clean complete!"
}

show_help() {
    cat << EOF
Usage: $0 [OPTIONS] [COMMAND]

Commands:
    build       Build the project (default)
    run         Build and run the application
    clean       Remove build directory
    rebuild     Clean and rebuild
    help        Show this help message

Options:
    --debug             Build with debug symbols
    --release           Build optimized release (default)
    --jobs N            Number of parallel build jobs (default: auto)
    --no-local-ftxui    Force download FTXUI even if local copy exists

Environment Variables:
    BUILD_DIR           Build directory (default: build)
    BUILD_TYPE          CMake build type (default: Release)
    JOBS                Number of parallel jobs (default: auto)

Examples:
    $0                  # Build release version
    $0 --debug          # Build debug version
    $0 run              # Build and run
    $0 clean build      # Clean rebuild
    $0 --jobs 2 build   # Build with 2 parallel jobs

EOF
}

# Parse command line arguments
COMMANDS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --no-local-ftxui)
            USE_FETCHCONTENT_FTXUI="ON"
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            COMMANDS+=("$1")
            shift
            ;;
    esac
done

# Default command is build
if [ ${#COMMANDS[@]} -eq 0 ]; then
    COMMANDS=("build")
fi

# Main execution
print_header

# Execute commands
for cmd in "${COMMANDS[@]}"; do
    case $cmd in
        build)
            check_dependencies
            setup_build_directory
            configure_cmake
            build_project
            ;;
        run)
            check_dependencies
            setup_build_directory
            configure_cmake
            build_project
            run_application
            ;;
        clean)
            clean_build
            ;;
        rebuild)
            clean_build
            check_dependencies
            setup_build_directory
            configure_cmake
            build_project
            ;;
        help)
            show_help
            ;;
        *)
            print_error "Unknown command: $cmd"
            echo
            show_help
            exit 1
            ;;
    esac
done

echo
print_success "All operations completed successfully!"