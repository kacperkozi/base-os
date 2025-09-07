#!/usr/bin/env bash

# Base OS TUI - Dependency Preparation Script
# Downloads and prepares FTXUI for offline/ISO deployment

set -e

# Configuration
FTXUI_VERSION="6.1.9"
VENDOR_DIR="vendor"
DEPS_DIR="deps"
ARCH=$(dpkg --print-architecture 2>/dev/null || echo "amd64")

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}   Dependency Preparation Script${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create necessary directories
setup_directories() {
    print_info "Creating directories..."
    mkdir -p "$VENDOR_DIR"
    mkdir -p "$DEPS_DIR/debs"
    mkdir -p "$DEPS_DIR/sources"
}

# Download FTXUI source tarball
download_ftxui_source() {
    print_info "Downloading FTXUI v${FTXUI_VERSION} source..."
    
    local tarball="ftxui-${FTXUI_VERSION}.tar.gz"
    local url="https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v${FTXUI_VERSION}.tar.gz"
    local target_dir="../third-party/FTXUI"
    
    # Check if FTXUI already exists in third-party
    if [ -d "$target_dir" ] && [ -f "$target_dir/CMakeLists.txt" ]; then
        print_success "FTXUI v${FTXUI_VERSION} already available in third-party/FTXUI"
        print_info "✅ Ready for ISO deployment!"
        return 0
    fi
    
    # Create third-party directory if it doesn't exist
    mkdir -p "../third-party"
    
    if [ -f "$VENDOR_DIR/$tarball" ]; then
        print_info "Tarball already exists, using cached version"
    else
        print_info "Downloading from GitHub..."
        if command -v wget &> /dev/null; then
            wget -O "$VENDOR_DIR/$tarball" "$url" || {
                print_error "Failed to download FTXUI source with wget"
                return 1
            }
        elif command -v curl &> /dev/null; then
            curl -L -o "$VENDOR_DIR/$tarball" "$url" || {
                print_error "Failed to download FTXUI source with curl"
                return 1
            }
        else
            print_error "Neither wget nor curl available for download"
            return 1
        fi
        print_success "Downloaded FTXUI source"
    fi
    
    # Extract to third-party directory
    print_info "Extracting FTXUI to third-party/FTXUI..."
    cd "$VENDOR_DIR"
    tar -xzf "$tarball"
    
    # Move to the correct location for CMakeLists.txt to find it
    if [ -d "FTXUI-${FTXUI_VERSION}" ]; then
        # Create third-party directory if it doesn't exist
        mkdir -p "../third-party"
        
        # Remove existing if present
        rm -rf "$target_dir"
        
        # Move extracted directory to target location
        mv "FTXUI-${FTXUI_VERSION}" "$target_dir"
        
        # Verify the move was successful
        if [ -f "$target_dir/CMakeLists.txt" ]; then
            print_success "FTXUI source prepared at $target_dir"
        else
            print_error "Failed to move FTXUI to target directory"
            return 1
        fi
    else
        print_error "Extraction failed - FTXUI-${FTXUI_VERSION} directory not found"
        print_info "Available directories:"
        ls -la
        return 1
    fi
    
    cd ..
}

# Build FTXUI as static libraries
build_ftxui_static() {
    print_info "Building FTXUI static libraries..."
    
    local src_dir="$VENDOR_DIR/ftxui-${FTXUI_VERSION}"
    local build_dir="$src_dir/build"
    
    if [ ! -d "$src_dir" ]; then
        print_error "FTXUI source not found. Run download first."
        return 1
    fi
    
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DFTXUI_BUILD_DOCS=OFF \
        -DFTXUI_BUILD_EXAMPLES=OFF \
        -DFTXUI_BUILD_TESTS=OFF \
        -DFTXUI_ENABLE_INSTALL=ON \
        -DCMAKE_INSTALL_PREFIX="$PWD/../../../$DEPS_DIR/ftxui-install"
    
    cmake --build . -j$(nproc)
    cmake --install .
    
    cd ../../..
    print_success "FTXUI built and installed to $DEPS_DIR/ftxui-install"
}

# Create a pseudo .deb package structure
create_deb_structure() {
    print_info "Creating .deb package structure..."
    
    local deb_name="libftxui-dev_${FTXUI_VERSION}_${ARCH}"
    local deb_dir="$DEPS_DIR/debs/$deb_name"
    
    mkdir -p "$deb_dir/DEBIAN"
    mkdir -p "$deb_dir/usr/include"
    mkdir -p "$deb_dir/usr/lib"
    
    # Copy headers and libraries
    if [ -d "$DEPS_DIR/ftxui-install" ]; then
        cp -r "$DEPS_DIR/ftxui-install/include/"* "$deb_dir/usr/include/"
        cp -r "$DEPS_DIR/ftxui-install/lib/"* "$deb_dir/usr/lib/"
    fi
    
    # Create control file
    cat > "$deb_dir/DEBIAN/control" << EOF
Package: libftxui-dev
Version: ${FTXUI_VERSION}
Architecture: ${ARCH}
Maintainer: Base OS TUI Team
Description: Fast Terminal User Interface library
 FTXUI is a simple C++ library for terminal based user interfaces.
 This package contains the development files.
Section: libdevel
Priority: optional
EOF
    
    # Build the .deb package
    print_info "Building .deb package..."
    dpkg-deb --build "$deb_dir" "$DEPS_DIR/debs/${deb_name}.deb" 2>/dev/null || {
        print_error "dpkg-deb not available, creating tar archive instead"
        tar -czf "$DEPS_DIR/debs/${deb_name}.tar.gz" -C "$deb_dir" .
    }
    
    print_success "Package created at $DEPS_DIR/debs/"
}

# Create offline installation script
create_install_script() {
    print_info "Creating offline installation script..."
    
    cat > "$DEPS_DIR/install-deps.sh" << 'EOF'
#!/bin/bash
# Offline dependency installation script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Installing dependencies for Base OS TUI..."

# Install from .deb files if available
if [ -d "$SCRIPT_DIR/debs" ]; then
    for deb in "$SCRIPT_DIR/debs"/*.deb; do
        if [ -f "$deb" ]; then
            echo "Installing $(basename "$deb")..."
            sudo dpkg -i "$deb" || sudo apt-get install -f -y
        fi
    done
fi

# Alternative: Extract tar archives
if [ -d "$SCRIPT_DIR/debs" ]; then
    for tar in "$SCRIPT_DIR/debs"/*.tar.gz; do
        if [ -f "$tar" ]; then
            echo "Extracting $(basename "$tar")..."
            sudo tar -xzf "$tar" -C /
        fi
    done
fi

echo "Dependencies installed successfully!"
EOF
    
    chmod +x "$DEPS_DIR/install-deps.sh"
    print_success "Installation script created"
}

# Create ISO-ready package
create_iso_package() {
    print_info "Creating ISO-ready package..."
    
    local iso_dir="iso-package"
    mkdir -p "$iso_dir"
    
    # Copy everything needed for offline build
    cp -r "$VENDOR_DIR" "$iso_dir/"
    cp -r "$DEPS_DIR" "$iso_dir/"
    cp -r src "$iso_dir/"
    cp -r include "$iso_dir/"
    cp CMakeLists.txt "$iso_dir/"
    cp Makefile "$iso_dir/"
    cp build.sh "$iso_dir/"
    
    # Create README for ISO
    cat > "$iso_dir/README.md" << EOF
# Base OS TUI - ISO Package

This package contains everything needed to build the Base OS TUI application offline.

## Contents
- vendor/: FTXUI source code (v${FTXUI_VERSION})
- deps/: Pre-built dependencies and installation scripts
- src/: Application source code
- include/: Application headers

## Building
1. For system installation: ./deps/install-deps.sh
2. Build application: make build

## Build Options
- USE_SYSTEM_FTXUI=ON: Use system-installed FTXUI
- USE_VENDORED_FTXUI=ON: Use included source (default)
EOF
    
    # Create tarball for ISO
    tar -czf "base-os-tui-iso-package-${FTXUI_VERSION}.tar.gz" "$iso_dir"
    
    print_success "ISO package created: base-os-tui-iso-package-${FTXUI_VERSION}.tar.gz"
}

# Main menu
show_menu() {
    echo
    echo "Select operation:"
    echo "1) Download FTXUI source"
    echo "2) Build FTXUI static libraries"
    echo "3) Create .deb package structure"
    echo "4) Create offline installation script"
    echo "5) Create ISO-ready package"
    echo "6) Do everything (recommended)"
    echo "0) Exit"
    echo
    read -p "Choice: " choice
    
    case $choice in
        1) download_ftxui_source ;;
        2) build_ftxui_static ;;
        3) create_deb_structure ;;
        4) create_install_script ;;
        5) create_iso_package ;;
        6) 
            setup_directories
            download_ftxui_source
            build_ftxui_static
            create_deb_structure
            create_install_script
            create_iso_package
            ;;
        0) exit 0 ;;
        *) print_error "Invalid choice" ;;
    esac
}

# Main execution
print_header

if [ "$1" == "--all" ] || [ "$1" == "-a" ]; then
    setup_directories
    download_ftxui_source
    build_ftxui_static
    create_deb_structure
    create_install_script
    create_iso_package
    print_success "All operations completed!"
else
    show_menu
fi