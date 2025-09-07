#!/usr/bin/env bash

# Docker-based ISO creator for macOS and other systems
# This script runs the ISO creation process inside a Debian container

set -e

# --- Colors for output ---
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

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    print_error "Docker is required but not installed."
    print_info "Please install Docker Desktop from: https://www.docker.com/products/docker-desktop"
    exit 1
fi

# Check if Docker daemon is running
if ! docker info &> /dev/null; then
    print_error "Docker daemon is not running."
    print_info "Please start Docker Desktop and try again."
    exit 1
fi

# Configuration
ARCH="${ARCH:-amd64}"
DEBIAN_VERSION="${DEBIAN_VERSION:-11.11}"
CONTAINER_NAME="iso-creator-$$"
WORK_DIR="$(pwd)"

print_info "========================================="
print_info "  Docker-based ISO Creator"
print_info "========================================="
print_info "Architecture: $ARCH"
print_info "Debian Version: $DEBIAN_VERSION"
print_info "Work Directory: $WORK_DIR"
print_info "========================================="

# Create Dockerfile
print_info "Creating Docker container for ISO creation..."
cat > Dockerfile.iso-creator << 'EOF'
FROM debian:12

# Install required packages
RUN apt-get update && apt-get install -y \
    wget \
    xorriso \
    squashfs-tools \
    rsync \
    isolinux \
    syslinux-common \
    file \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /work

# Copy the repository
COPY . /work/

# Make scripts executable
RUN chmod +x create_custom_iso.sh

# Run as root (required for ISO creation)
USER root

EOF

# Build Docker image
print_info "Building Docker image..."
docker build -f Dockerfile.iso-creator -t iso-creator:latest . || {
    print_error "Failed to build Docker image"
    rm -f Dockerfile.iso-creator
    exit 1
}

# Clean up Dockerfile
rm -f Dockerfile.iso-creator

# Run the ISO creation in Docker
print_info "Running ISO creation in Docker container..."
print_info "This will take 15-30 minutes depending on download speed..."
print_info ""

docker run --rm \
    --name "$CONTAINER_NAME" \
    -v "$WORK_DIR:/work" \
    -e ARCH="$ARCH" \
    -e DEBIAN_VERSION="$DEBIAN_VERSION" \
    --privileged \
    --cap-add SYS_ADMIN \
    --device /dev/loop0 \
    --device /dev/loop1 \
    --device /dev/loop2 \
    --device /dev/loop3 \
    --device /dev/loop4 \
    --device /dev/loop5 \
    --device /dev/loop6 \
    --device /dev/loop7 \
    iso-creator:latest \
    bash -c "cd /work && ./create_custom_iso.sh"

# Check if ISO was created
ISO_FILE="custom-debian-live-${ARCH}.iso"
if [ -f "$ISO_FILE" ]; then
    print_success "========================================="
    print_success "  ISO Creation Complete!"
    print_success "========================================="
    print_success "Output file: $ISO_FILE"
    print_success "Size: $(du -h $ISO_FILE | cut -f1)"
    print_success "========================================="
    print_info "To test the ISO:"
    print_info "  - For VM: Use VirtualBox, VMware, or UTM (for Apple Silicon)"
    print_info "  - For physical: Write to USB with balenaEtcher or dd"
else
    print_error "ISO creation failed. Check the output above for errors."
    exit 1
fi