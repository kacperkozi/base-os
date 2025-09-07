#!/usr/bin/env bash

# UTM VM Setup Script for ISO Creation
# This script helps you set up a Debian VM in UTM to create the custom ISO

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

print_info "========================================="
print_info "  UTM VM Setup for ISO Creation"
print_info "========================================="

# Check if UTM is installed
if [ -d "/Applications/UTM.app" ]; then
    print_success "UTM found at /Applications/UTM.app"
else
    print_warning "UTM not found. Please install UTM from:"
    print_info "  https://mac.getutm.app/"
    print_info "  or via Homebrew: brew install --cask utm"
    exit 1
fi

print_info ""
print_info "Step 1: Create a new Debian 11 VM in UTM"
print_info "========================================="
print_info "1. Open UTM"
print_info "2. Click '+' to create new VM"
print_info "3. Choose 'Virtualize' (for Apple Silicon) or 'Emulate' (for Intel)"
print_info "4. Select 'Linux'"
print_info "5. Use these settings:"
print_info "   - ISO: debian-11.11.0-amd64-netinst.iso (download from debian.org)"
print_info "   - RAM: 4096 MB minimum"
print_info "   - Storage: 20 GB minimum"
print_info "   - CPU: 2-4 cores"
print_info "   - Enable hardware acceleration if available"
print_info ""

print_info "Step 2: Install Debian 11 in the VM"
print_info "========================================="
print_info "1. Boot from the Debian installer ISO"
print_info "2. Install Debian with standard options"
print_info "3. Install SSH server during setup"
print_info "4. Create a user account (e.g., 'builder')"
print_info ""

print_info "Step 3: Configure the VM for ISO Creation"
print_info "========================================="
print_info "After Debian is installed, run these commands in the VM:"
print_info ""
cat << 'EOF'
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y \
    wget \
    xorriso \
    squashfs-tools \
    rsync \
    isolinux \
    syslinux-common \
    build-essential \
    cmake \
    git \
    file

# Create work directory
mkdir -p ~/iso-build
cd ~/iso-build
EOF

print_info ""
print_info "Step 4: Transfer Files to VM"
print_info "========================================="
print_info "Option A: Using SCP (if SSH is enabled):"
print_info "  1. Find VM IP: ip addr show"
print_info "  2. From macOS, transfer files:"
print_success "     scp -r $(pwd) builder@VM_IP:~/iso-build/"
print_success "     scp debian-live-11.11.0-amd64-xfce.iso builder@VM_IP:~/iso-build/"
print_info ""
print_info "Option B: Using UTM Shared Directory:"
print_info "  1. In UTM, edit VM settings"
print_info "  2. Add a shared directory pointing to: $(pwd)"
print_info "  3. Mount in VM: sudo mount -t 9p -o trans=virtio share /mnt/share"
print_info ""

print_info "Step 5: Run ISO Creator in VM"
print_info "========================================="
print_info "SSH into the VM or use the console:"
print_info ""
cat << 'EOF'
cd ~/iso-build/base-os
sudo ARCH=amd64 DEBIAN_VERSION=11.11 ./create_custom_iso.sh
EOF

print_info ""
print_info "Step 6: Retrieve the Custom ISO"
print_info "========================================="
print_info "After creation, copy the ISO back to macOS:"
print_success "  scp builder@VM_IP:~/iso-build/base-os/custom-debian-live-amd64.iso ./"
print_info ""

# Create a script for the VM
cat > vm_iso_builder.sh << 'SCRIPT_EOF'
#!/bin/bash
# Run this script inside the Debian VM

set -e

echo "==================================="
echo "  ISO Builder for Debian VM"
echo "==================================="

# Install dependencies
echo "Installing required packages..."
sudo apt update
sudo apt install -y wget xorriso squashfs-tools rsync isolinux syslinux-common build-essential cmake git

# Check if base-os directory exists
if [ ! -d "base-os" ]; then
    echo "ERROR: base-os directory not found!"
    echo "Please copy the base-os repository to this VM"
    exit 1
fi

# Check if Debian ISO exists
if [ ! -f "debian-live-11.11.0-amd64-xfce.iso" ]; then
    echo "ERROR: Debian Live ISO not found!"
    echo "Please copy debian-live-11.11.0-amd64-xfce.iso to this directory"
    exit 1
fi

# Copy ISO to base-os directory
cp debian-live-11.11.0-amd64-xfce.iso base-os/

# Run the ISO creator
cd base-os
sudo ARCH=amd64 DEBIAN_VERSION=11.11 ./create_custom_iso.sh

echo "==================================="
echo "  ISO Creation Complete!"
echo "==================================="
echo "Output file: custom-debian-live-amd64.iso"
ls -lh custom-debian-live-amd64.iso
SCRIPT_EOF

chmod +x vm_iso_builder.sh

print_success "========================================="
print_success "  Setup Instructions Complete!"
print_success "========================================="
print_success "Created: vm_iso_builder.sh"
print_info "Copy this script to the VM and run it after transferring files"
print_info ""
print_info "Files to transfer to VM:"
print_info "  1. This repository (base-os/)"
print_info "  2. debian-live-11.11.0-amd64-xfce.iso"
print_info "  3. vm_iso_builder.sh"
print_info ""
print_success "Ready to create VM and build ISO!"