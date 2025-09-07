#!/usr/bin/env bash

# QEMU ISO Builder - Run Debian VM with QEMU directly
# This creates and runs a temporary Debian VM to build the ISO

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

# Check for QEMU
if ! command -v qemu-system-x86_64 &> /dev/null; then
    print_error "QEMU not found. Installing..."
    if command -v brew &> /dev/null; then
        brew install qemu
    else
        print_error "Please install QEMU first:"
        print_info "  brew install qemu"
        exit 1
    fi
fi

# Configuration
VM_DISK="debian-vm.qcow2"
VM_MEMORY="4096"
VM_CPUS="2"
DEBIAN_INSTALLER="debian-11.11.0-amd64-netinst.iso"
WORK_DIR="$(pwd)"

print_info "========================================="
print_info "  QEMU ISO Builder"
print_info "========================================="
print_info "VM Disk: $VM_DISK"
print_info "Memory: ${VM_MEMORY}MB"
print_info "CPUs: $VM_CPUS"
print_info "========================================="

# Download Debian netinst if not present
if [ ! -f "$DEBIAN_INSTALLER" ]; then
    print_info "Downloading Debian 11 netinst installer..."
    wget -O "$DEBIAN_INSTALLER" \
        "https://cdimage.debian.org/cdimage/archive/11.11.0/amd64/iso-cd/debian-11.11.0-amd64-netinst.iso"
fi

# Create VM disk if not exists
if [ ! -f "$VM_DISK" ]; then
    print_info "Creating VM disk image (20GB)..."
    qemu-img create -f qcow2 "$VM_DISK" 20G
    
    print_info "========================================="
    print_info "  First Run: Install Debian"
    print_info "========================================="
    print_info "1. VM will boot from installer ISO"
    print_info "2. Install Debian with:"
    print_info "   - Standard system utilities"
    print_info "   - SSH server"
    print_info "   - Create user 'builder'"
    print_info "3. After installation, shutdown the VM"
    print_info "========================================="
    print_warning "Press Enter to start VM installation..."
    read -r
    
    # Run QEMU for installation
    qemu-system-x86_64 \
        -machine type=q35,accel=hvf \
        -cpu host \
        -smp "$VM_CPUS" \
        -m "$VM_MEMORY" \
        -drive file="$VM_DISK",if=virtio,format=qcow2 \
        -cdrom "$DEBIAN_INSTALLER" \
        -boot d \
        -display default \
        -device virtio-net-pci,netdev=net0 \
        -netdev user,id=net0,hostfwd=tcp::2222-:22
    
    print_success "Debian installation complete!"
fi

# Create automated build script
cat > vm_build_commands.sh << 'EOF'
#!/bin/bash
# Commands to run inside the VM

set -e

# Update and install packages
sudo apt update
sudo apt install -y wget xorriso squashfs-tools rsync isolinux syslinux-common build-essential cmake git

# Mount shared folder (if using 9p)
if [ -d "/mnt/share" ]; then
    echo "Shared folder found"
else
    sudo mkdir -p /mnt/share
fi

# Build the ISO
cd /home/builder
if [ -f /mnt/share/debian-live-11.11.0-amd64-xfce.iso ]; then
    cp /mnt/share/debian-live-11.11.0-amd64-xfce.iso ./
fi
if [ -d /mnt/share/base-os ]; then
    cp -r /mnt/share/base-os ./
fi

cd base-os
sudo ARCH=amd64 DEBIAN_VERSION=11.11 ./create_custom_iso.sh

# Copy result back
cp custom-debian-live-amd64.iso /mnt/share/
echo "ISO created successfully!"
EOF

print_info "========================================="
print_info "  Run VM for ISO Building"
print_info "========================================="
print_info "Starting VM with shared folder..."
print_info "SSH will be available on port 2222"
print_info "========================================="

# Run QEMU with shared folder
print_info "Starting QEMU VM..."
print_info "To connect via SSH: ssh -p 2222 builder@localhost"
print_info "To transfer files: scp -P 2222 file.iso builder@localhost:~/"
print_info ""

# Create a startup script for the VM
cat > start_vm.sh << EOF
#!/bin/bash
qemu-system-x86_64 \\
    -machine type=q35,accel=hvf \\
    -cpu host \\
    -smp $VM_CPUS \\
    -m $VM_MEMORY \\
    -drive file="$VM_DISK",if=virtio,format=qcow2 \\
    -display default \\
    -device virtio-net-pci,netdev=net0 \\
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \\
    -virtfs local,path=$WORK_DIR,mount_tag=share,security_model=mapped,id=share \\
    -nographic
EOF

chmod +x start_vm.sh

print_success "========================================="
print_success "  Setup Complete!"
print_success "========================================="
print_info "Created files:"
print_success "  - start_vm.sh: Script to start the VM"
print_success "  - vm_build_commands.sh: Commands to run in VM"
print_info ""
print_info "Next steps:"
print_info "1. Run: ./start_vm.sh"
print_info "2. SSH into VM: ssh -p 2222 builder@localhost"
print_info "3. Transfer files:"
print_success "   scp -P 2222 debian-live-11.11.0-amd64-xfce.iso builder@localhost:~/"
print_success "   scp -P 2222 -r base-os builder@localhost:~/"
print_info "4. Run ISO builder in VM:"
print_success "   cd ~/base-os"
print_success "   sudo ./create_custom_iso.sh"
print_info "5. Copy ISO back:"
print_success "   scp -P 2222 builder@localhost:~/base-os/custom-debian-live-amd64.iso ./"
print_info ""
print_warning "Note: First boot will be slower as it sets up the VM"