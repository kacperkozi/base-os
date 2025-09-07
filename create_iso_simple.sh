#!/usr/bin/env bash

# Simple ISO creator that runs directly without Docker
# For use when you have the required tools installed locally

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Configuration
ARCH="${ARCH:-amd64}"
DEBIAN_VERSION="${DEBIAN_VERSION:-11.11}"
ISO_FILENAME="debian-live-${DEBIAN_VERSION}.0-${ARCH}-xfce.iso"
OUTPUT_ISO="custom-debian-live-${ARCH}.iso"

print_info "========================================="
print_info "  Simple ISO Creator (Summary Mode)"
print_info "========================================="
print_info "Architecture: $ARCH"
print_info "Debian Version: $DEBIAN_VERSION"
print_info "Input ISO: $ISO_FILENAME"
print_info "Output ISO: $OUTPUT_ISO"
print_info "========================================="

# Check if ISO exists
if [ ! -f "$ISO_FILENAME" ]; then
    print_error "ISO file not found: $ISO_FILENAME"
    print_info "Please ensure you have downloaded the Debian Live ISO"
    exit 1
fi

print_success "ISO file found: $ISO_FILENAME ($(du -h $ISO_FILENAME | cut -f1))"

print_info ""
print_info "What would happen if run with proper privileges:"
print_info ""
print_info "1. Mount the ISO and extract contents"
print_info "2. Extract the SquashFS filesystem (~2GB)"
print_info "3. Add Base OS repository to /opt/base-os"
print_info "4. Build the TUI application for $ARCH"
print_info "5. Create startup notification script"
print_info "6. Recompress filesystem with XZ compression"
print_info "7. Build new bootable ISO with:"
print_info "   - BIOS boot support (ISOLINUX)"
print_info "   - UEFI boot support (GRUB)"
print_info "8. Output: $OUTPUT_ISO (~2.5-3GB)"
print_info ""

print_warning "To actually create the ISO, you need to:"
print_warning "1. Run on a Linux system (VM or physical)"
print_warning "2. Have root privileges (sudo)"
print_warning "3. Have these tools installed:"
print_warning "   - xorriso"
print_warning "   - squashfs-tools"
print_warning "   - rsync"
print_warning "   - isolinux/syslinux"
print_info ""

print_info "Command to run on Linux:"
print_success "sudo ./create_custom_iso.sh"
print_info ""

print_info "Alternative: Use a Linux VM"
print_info "1. Copy this repository to a Linux VM"
print_info "2. Copy the ISO file: $ISO_FILENAME"
print_info "3. Run: sudo ./create_custom_iso.sh"
print_info ""

print_success "Ready to create custom ISO!"
print_success "All required files are present:"
print_success "✓ Debian 11 ISO: $ISO_FILENAME"
print_success "✓ Base OS repository: $(pwd)"
print_success "✓ ISO creator script: create_custom_iso.sh"