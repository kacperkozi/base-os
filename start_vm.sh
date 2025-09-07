#!/bin/bash
qemu-system-x86_64 \
    -machine type=q35,accel=hvf \
    -cpu host \
    -smp 2 \
    -m 4096 \
    -drive file="debian-vm.qcow2",if=virtio,format=qcow2 \
    -display default \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -virtfs local,path=/Users/agent/Projects/prod_ethwarsaw/base-os,mount_tag=share,security_model=mapped,id=share \
    -nographic
