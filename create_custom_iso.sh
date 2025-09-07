#!/usr/bin/env bash

# This script creates a modified Debian Live ISO that includes the current repository.
# It must be run as root.

set -e

# --- Configuration ---
ISO_URL="https://img.cs.montana.edu/linux/debian/11/amd/debian-live-11.0.0-amd64-xfce.iso"
ISO_FILENAME="debian-live-11.0.0-amd64-xfce.iso"
REPO_PATH="$(pwd)"

# --- Check for root privileges ---
if [ "$(id -u)" -ne 0 ]; then
  echo "This script must be run as root." >&2
  exit 1
fi

# --- Install necessary tools ---
echo "[*] Installing necessary tools..."
apt-get update
apt-get install -y wget xorriso squashfs-tools rsync

# --- Download the base ISO ---
if [ ! -f "$ISO_FILENAME" ]; then
  echo "[*] Downloading Debian Live ISO..."
  wget -O "$ISO_FILENAME" "$ISO_URL"
fi

# --- Set up working directories ---
rm -rf iso_contents squashfs-root
mkdir -p /mnt/iso iso_contents squashfs-root

# --- Mount the ISO ---
echo "[*] Mounting the ISO..."
mount -o loop "$ISO_FILENAME" /mnt/iso

# --- Extract the ISO contents ---
echo "[*] Extracting the ISO contents..."
rsync -a /mnt/iso/ iso_contents/

# --- Extract the SquashFS filesystem ---
echo "[*] Extracting the SquashFS filesystem..."
unsquashfs iso_contents/live/filesystem.squashfs

# --- Copy the repository ---
echo "[*] Copying the repository..."
rsync -a "$REPO_PATH" squashfs-root/

# --- Re-create the SquashFS filesystem ---
echo "[*] Re-creating the SquashFS filesystem..."
mksquashfs squashfs-root filesystem.squashfs -comp xz -e boot

# --- Re-create the ISO ---
echo "[*] Re-creating the ISO..."
xorriso -as mkisofs \
  -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
  -c isolinux/boot.cat \
  -b isolinux/isolinux.bin \
  -no-emul-boot \
  -boot-load-size 4 \
  -boot-info-table \
  -eltorito-alt-boot \
  -e boot/grub/efi.img \
  -no-emul-boot \
  -isohybrid-gpt-basdat \
  -o custom-debian-live.iso \
  iso_contents

# --- Clean up ---
echo "[*] Cleaning up..."
umount /mnt/iso
rm -rf iso_contents squashfs-root

echo "[+] Custom ISO created successfully: custom-debian-live.iso"
