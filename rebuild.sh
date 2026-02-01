#!/bin/bash
# Rebuild script after trap.c fix
cd /workspaces/unix-v6/kernel
make clean
make build
echo "Build complete!"
echo "Rebuilding ISO..."
make iso
echo "ISO created at unix_v6.iso"
