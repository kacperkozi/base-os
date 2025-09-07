#!/bin/bash
# Automated ISO creation using QEMU VM

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

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
print_info "  Automated ISO Creation in QEMU VM"
print_info "========================================="

# Check for required files
if [ ! -f "debian-live-11.11.0-amd64-xfce.iso" ]; then
    print_error "Missing debian-live-11.11.0-amd64-xfce.iso"
    print_info "This file is required for ISO creation"
    exit 1
fi

if [ ! -f "debian-11.11.0-amd64-netinst.iso" ]; then
    print_error "Missing debian-11.11.0-amd64-netinst.iso"
    print_info "This file is required for VM installation"
    exit 1
fi

print_success "All required ISO files found"

# Option 1: Use existing VM if available
if [ -f "debian-vm.qcow2" ] && [ $(stat -f%z "debian-vm.qcow2" 2>/dev/null || stat -c%s "debian-vm.qcow2" 2>/dev/null) -gt 1000000 ]; then
    print_info "Existing VM disk found, attempting to use it"
    print_info "Starting VM..."
    
    # Start VM in background
    print_info "Starting QEMU VM in background..."
    qemu-system-x86_64 \
        -machine type=q35,accel=hvf \
        -cpu host \
        -smp 2 \
        -m 4096 \
        -drive file="debian-vm.qcow2",if=virtio,format=qcow2 \
        -device virtio-net-pci,netdev=net0 \
        -netdev user,id=net0,hostfwd=tcp::2222-:22 \
        -nographic \
        -daemonize \
        -pidfile qemu.pid \
        -serial mon:stdio &
    
    QEMU_PID=$!
    print_info "QEMU started with PID: $QEMU_PID"
    
    print_info "Waiting for VM to boot (30 seconds)..."
    sleep 30
    
    # Try to connect via SSH
    print_info "Attempting SSH connection..."
    if ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no -p 2222 builder@localhost "echo 'SSH connection successful'" 2>/dev/null; then
        print_success "SSH connection established!"
        
        # Transfer files
        print_info "Transferring files to VM..."
        scp -P 2222 -o StrictHostKeyChecking=no debian-live-11.11.0-amd64-xfce.iso builder@localhost:~/ || true
        scp -P 2222 -o StrictHostKeyChecking=no -r . builder@localhost:~/base-os/ || true
        
        # Run ISO creation
        print_info "Running ISO creation in VM..."
        ssh -p 2222 -o StrictHostKeyChecking=no builder@localhost << 'EOF'
        cd ~/base-os
        sudo apt-get update
        sudo apt-get install -y wget xorriso squashfs-tools rsync isolinux syslinux-common build-essential cmake git
        sudo ARCH=amd64 DEBIAN_VERSION=11.11 ./create_custom_iso.sh
EOF
        
        # Copy ISO back
        print_info "Copying ISO back from VM..."
        scp -P 2222 -o StrictHostKeyChecking=no builder@localhost:~/base-os/custom-debian-live-amd64.iso ./
        
        print_success "ISO creation complete!"
        
        # Stop VM
        if [ -f qemu.pid ]; then
            kill $(cat qemu.pid) 2>/dev/null || true
            rm -f qemu.pid
        fi
    else
        print_warning "SSH connection failed. VM may need to be installed first."
        print_info "Please run: ./install_debian_vm.sh"
        print_info "Then run this script again."
        
        # Stop VM
        if [ -f qemu.pid ]; then
            kill $(cat qemu.pid) 2>/dev/null || true
            rm -f qemu.pid
        fi
        exit 1
    fi
else
    print_warning "No existing VM found or VM disk too small"
    print_info "Please run: ./install_debian_vm.sh first"
    print_info "This will install Debian 11 in a VM"
    print_info "Then run this script again to create the ISO"
    exit 1
fi

print_success "========================================="
print_success "  ISO Creation Complete!"
print_success "========================================="
print_success "Output: custom-debian-live-amd64.iso"
ls -lh custom-debian-live-amd64.iso 2>/dev/null || true