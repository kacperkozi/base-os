#!/usr/bin/env bash

# This script creates a modified Debian Live ISO that includes the current repository.
# Supports both AMD64 and ARM64 architectures.
# Must be run as root.

set -e

# --- Colors for output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# --- Functions ---
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

# --- Detect system architecture ---
detect_architecture() {
    local arch=$(uname -m)
    case "$arch" in
        x86_64|amd64)
            echo "amd64"
            ;;
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            print_error "Unsupported architecture: $arch"
            exit 1
            ;;
    esac
}

# --- Configuration ---
ARCH="${ARCH:-$(detect_architecture)}"
DEBIAN_VERSION="${DEBIAN_VERSION:-11.11}"
REPO_PATH="$(pwd)"
WORK_DIR="${WORK_DIR:-./iso_work}"
OUTPUT_ISO="custom-debian-live-${ARCH}.iso"

# Architecture-specific ISO URLs
case "$ARCH" in
    amd64)
        ISO_URL="https://cdimage.debian.org/debian-cd/current-live/amd64/iso-hybrid/debian-live-${DEBIAN_VERSION}.0-amd64-xfce.iso"
        ;;
    arm64)
        ISO_URL="https://cdimage.debian.org/debian-cd/current-live/arm64/iso-hybrid/debian-live-${DEBIAN_VERSION}.0-arm64-xfce.iso"
        ;;
    *)
        print_error "Unsupported architecture: $ARCH"
        print_info "Supported architectures: amd64, arm64"
        exit 1
        ;;
esac
ISO_FILENAME="debian-live-${DEBIAN_VERSION}.0-${ARCH}-xfce.iso"

print_info "========================================="
print_info "  Debian Live ISO Creator"
print_info "========================================="
print_info "Architecture: $ARCH"
print_info "Debian Version: $DEBIAN_VERSION"
print_info "Repository Path: $REPO_PATH"
print_info "Output ISO: $OUTPUT_ISO"
print_info "========================================="

# --- Check for root privileges ---
if [ "$(id -u)" -ne 0 ]; then
    print_error "This script must be run as root."
    print_info "Try: sudo $0"
    exit 1
fi

# --- Install necessary tools ---
print_info "Checking and installing necessary tools..."

# Detect the operating system
OS_TYPE="$(uname -s)"

if [ "$OS_TYPE" = "Darwin" ]; then
    # macOS detected
    print_warning "macOS detected - checking for required tools..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        print_error "Homebrew is required on macOS. Please install from https://brew.sh"
        exit 1
    fi
    
    # Check and install required tools
    REQUIRED_TOOLS="wget xorriso squashfs rsync"
    MISSING_TOOLS=""
    
    for tool in $REQUIRED_TOOLS; do
        # Special case for squashfs-tools which is just 'squashfs' in Homebrew
        check_tool="$tool"
        if [ "$tool" = "squashfs" ]; then
            check_tool="mksquashfs"
        fi
        
        if ! command -v "$check_tool" &> /dev/null; then
            MISSING_TOOLS="$MISSING_TOOLS $tool"
        else
            print_success "  ✓ $tool found"
        fi
    done
    
    if [ -n "$MISSING_TOOLS" ]; then
        print_info "Installing missing tools:$MISSING_TOOLS"
        brew install $MISSING_TOOLS
    fi
    
    print_warning "Note: Creating Linux ISOs on macOS has limitations:"
    print_warning "  - Cannot mount Linux filesystems natively"
    print_warning "  - Cannot chroot into Linux environments"
    print_warning "  - Recommended to use a Linux VM or Docker container"
    print_info ""
    print_info "Alternative: Use Docker to run this script in a Linux container:"
    print_info "  docker run -it -v \$(pwd):/work debian:12 bash"
    print_info "  cd /work && ./create_custom_iso.sh"
    print_info ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Exiting. Consider using a Linux environment."
        exit 0
    fi
    
elif command -v apt-get &> /dev/null; then
    # Debian/Ubuntu
    apt-get update
    apt-get install -y wget xorriso squashfs-tools rsync isolinux syslinux-common
elif command -v yum &> /dev/null; then
    # Red Hat/CentOS/Fedora
    yum install -y wget xorriso squashfs-tools rsync syslinux
else
    print_error "Unsupported package manager. Please manually install: wget xorriso squashfs-tools rsync"
    print_info "Or run this script in a Docker container:"
    print_info "  docker run -it -v \$(pwd):/work debian:12 bash"
    exit 1
fi

# --- Download the base ISO ---
if [ ! -f "$ISO_FILENAME" ]; then
    print_info "Downloading Debian Live ISO for $ARCH..."
    print_info "URL: $ISO_URL"
    if ! wget --progress=bar:force -O "$ISO_FILENAME" "$ISO_URL"; then
        print_error "Failed to download ISO"
        print_info "Please check your internet connection and the URL: $ISO_URL"
        exit 1
    fi
    print_success "ISO downloaded successfully"
else
    print_info "Using existing ISO: $ISO_FILENAME"
fi

# --- Set up working directories ---
print_info "Setting up working directories..."
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"/{iso_mount,iso_contents,squashfs-root,new_iso}

# --- Mount the ISO ---
print_info "Mounting the ISO..."
if ! mount -o loop "$ISO_FILENAME" "$WORK_DIR/iso_mount"; then
    print_error "Failed to mount ISO"
    exit 1
fi

# --- Extract the ISO contents ---
print_info "Extracting ISO contents..."
rsync -a "$WORK_DIR/iso_mount/" "$WORK_DIR/iso_contents/"

# --- Extract the SquashFS filesystem ---
print_info "Extracting SquashFS filesystem..."
SQUASHFS_PATH="$WORK_DIR/iso_contents/live/filesystem.squashfs"
if [ ! -f "$SQUASHFS_PATH" ]; then
    print_error "SquashFS file not found at: $SQUASHFS_PATH"
    umount "$WORK_DIR/iso_mount"
    exit 1
fi

cd "$WORK_DIR"
unsquashfs -d squashfs-root "$SQUASHFS_PATH"
cd - > /dev/null

# --- Copy the repository ---
print_info "Copying repository into filesystem..."
REPO_NAME=$(basename "$REPO_PATH")
cp -r "$REPO_PATH" "$WORK_DIR/squashfs-root/opt/"
print_success "Repository copied to /opt/$REPO_NAME"

# --- Build the TUI application for the target architecture ---
print_info "Building TUI application for $ARCH..."
BUILD_SCRIPT="$WORK_DIR/squashfs-root/opt/$REPO_NAME/ui/tui/build.sh"
if [ -f "$BUILD_SCRIPT" ]; then
    # Create a build script for chroot environment
    cat > "$WORK_DIR/squashfs-root/tmp/build_tui.sh" << 'EOF'
#!/bin/bash
cd /opt/base-os/ui/tui
if [ -f build.sh ]; then
    chmod +x build.sh
    ./build.sh --release
fi
EOF
    chmod +x "$WORK_DIR/squashfs-root/tmp/build_tui.sh"
    
    # Try to build in chroot if possible
    if [ "$ARCH" = "$(detect_architecture)" ]; then
        print_info "Building natively for $ARCH..."
        chroot "$WORK_DIR/squashfs-root" /tmp/build_tui.sh || print_warning "Build failed in chroot, ISO will contain source only"
    else
        print_warning "Cross-architecture build not supported, ISO will contain source code only"
        print_info "Users will need to build the application after booting the ISO"
    fi
fi

# --- Create startup script ---
print_info "Creating startup script..."
cat > "$WORK_DIR/squashfs-root/etc/profile.d/base-os.sh" << 'EOF'
#!/bin/bash
# Base OS startup notification
if [ -d /opt/base-os ]; then
    echo ""
    echo "========================================="
    echo "  Base OS Repository Available"
    echo "========================================="
    echo "Location: /opt/base-os"
    echo "To build the TUI: cd /opt/base-os/ui/tui && ./build.sh"
    echo "========================================="
    echo ""
fi
EOF
chmod +x "$WORK_DIR/squashfs-root/etc/profile.d/base-os.sh"

# --- Re-create the SquashFS filesystem ---
print_info "Re-creating SquashFS filesystem..."
mksquashfs "$WORK_DIR/squashfs-root" "$WORK_DIR/new_iso/filesystem.squashfs" \
    -comp xz -b 1M -Xdict-size 100% \
    || {
        print_error "Failed to create SquashFS"
        umount "$WORK_DIR/iso_mount"
        exit 1
    }

# --- Replace the old SquashFS ---
print_info "Replacing SquashFS in ISO contents..."
rm -f "$WORK_DIR/iso_contents/live/filesystem.squashfs"
mv "$WORK_DIR/new_iso/filesystem.squashfs" "$WORK_DIR/iso_contents/live/"

# --- Find the correct boot files ---
print_info "Locating boot files..."
ISOLINUX_BIN=""
ISOHDPFX_BIN=""

# Search for isolinux.bin
for path in /usr/lib/ISOLINUX /usr/share/syslinux /usr/lib/syslinux/modules/bios; do
    if [ -f "$path/isolinux.bin" ]; then
        ISOLINUX_BIN="$path/isolinux.bin"
        break
    fi
done

# Search for isohdpfx.bin
for path in /usr/lib/ISOLINUX /usr/share/syslinux /usr/lib/syslinux/mbr; do
    if [ -f "$path/isohdpfx.bin" ]; then
        ISOHDPFX_BIN="$path/isohdpfx.bin"
        break
    fi
done

# --- Re-create the ISO ---
print_info "Creating new ISO: $OUTPUT_ISO"

# Build xorriso command based on architecture
XORRISO_CMD="xorriso -as mkisofs"

# Common options
XORRISO_CMD="$XORRISO_CMD -r -V 'Debian Live Base OS' -cache-inodes -J -l"

# Architecture-specific boot options
if [ "$ARCH" = "amd64" ]; then
    if [ -n "$ISOHDPFX_BIN" ]; then
        XORRISO_CMD="$XORRISO_CMD -isohybrid-mbr $ISOHDPFX_BIN"
    fi
    XORRISO_CMD="$XORRISO_CMD -b isolinux/isolinux.bin -c isolinux/boot.cat"
    XORRISO_CMD="$XORRISO_CMD -no-emul-boot -boot-load-size 4 -boot-info-table"
    
    # EFI support if available
    if [ -f "$WORK_DIR/iso_contents/boot/grub/efi.img" ]; then
        XORRISO_CMD="$XORRISO_CMD -eltorito-alt-boot -e boot/grub/efi.img -no-emul-boot"
        XORRISO_CMD="$XORRISO_CMD -isohybrid-gpt-basdat"
    fi
elif [ "$ARCH" = "arm64" ]; then
    # ARM64 typically uses UEFI only
    if [ -f "$WORK_DIR/iso_contents/boot/grub/efi.img" ]; then
        XORRISO_CMD="$XORRISO_CMD -e boot/grub/efi.img -no-emul-boot"
    else
        print_warning "No EFI boot image found for ARM64, ISO may not be bootable"
    fi
fi

XORRISO_CMD="$XORRISO_CMD -o $OUTPUT_ISO $WORK_DIR/iso_contents"

# Execute the command
print_info "Running: $XORRISO_CMD"
if eval $XORRISO_CMD; then
    print_success "ISO created successfully!"
else
    print_error "Failed to create ISO"
    umount "$WORK_DIR/iso_mount"
    exit 1
fi

# --- Clean up ---
print_info "Cleaning up..."
umount "$WORK_DIR/iso_mount" 2>/dev/null || true
if [ "${KEEP_WORK_DIR:-0}" = "0" ]; then
    rm -rf "$WORK_DIR"
    print_info "Work directory removed"
else
    print_info "Work directory kept at: $WORK_DIR"
fi

# --- Final summary ---
print_success "========================================="
print_success "  ISO Creation Complete!"
print_success "========================================="
print_success "Output file: $OUTPUT_ISO"
print_success "Architecture: $ARCH"
print_success "Size: $(du -h $OUTPUT_ISO | cut -f1)"
print_success "========================================="
print_info "To test the ISO:"
print_info "  - For VM: Use VirtualBox, VMware, or QEMU"
print_info "  - For physical: Write to USB with dd or Etcher"
print_info "========================================="
