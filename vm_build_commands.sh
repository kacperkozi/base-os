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
