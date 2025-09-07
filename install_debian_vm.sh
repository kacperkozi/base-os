#!/bin/bash
# Install Debian 11 in QEMU VM

set -e

echo "Starting Debian 11 installation in QEMU VM..."
echo "=========================================="
echo "Please follow these steps:"
echo "1. Choose 'Install' (not graphical install)"
echo "2. Select language, location, keyboard"
echo "3. Configure network (use DHCP)"
echo "4. Set hostname: debian-iso-builder"
echo "5. Create user: builder"
echo "6. Set a password you'll remember"
echo "7. Partition: use entire disk, all files in one partition"
echo "8. Select mirror close to you"
echo "9. Software selection:"
echo "   - UNCHECK 'Debian desktop environment'"
echo "   - UNCHECK 'GNOME'"
echo "   - CHECK 'SSH server'"
echo "   - CHECK 'standard system utilities'"
echo "10. Install GRUB to /dev/vda"
echo "11. After installation, system will reboot"
echo "12. Close QEMU when you see login prompt"
echo "=========================================="
echo "Press Enter to start installation..."
read -r

# Run QEMU with installer ISO
qemu-system-x86_64 \
    -machine type=q35,accel=hvf \
    -cpu host \
    -smp 2 \
    -m 4096 \
    -drive file="debian-vm.qcow2",if=virtio,format=qcow2 \
    -cdrom debian-11.11.0-amd64-netinst.iso \
    -boot d \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -display default

echo "Installation complete!"
echo "Now you can run: ./start_vm.sh"
echo "Then SSH in: ssh -p 2222 builder@localhost"