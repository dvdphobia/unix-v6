#!/bin/bash
# Complete rebuild script with all fixes applied

set -e

cd /workspaces/unix-v6/kernel

echo "=========================================="
echo "Building Unix V6 x86 Kernel"
echo "=========================================="
echo ""

echo "Step 1: Cleaning old build artifacts..."
make clean > /dev/null 2>&1 || true
echo "✅ Cleaned"

echo ""
echo "Step 2: Building kernel..."
if make build > build.log 2>&1; then
    echo "✅ Kernel built successfully"
else
    echo "❌ Build failed! Check build.log"
    cat build.log
    exit 1
fi

echo ""
echo "Step 3: Creating ISO image..."
if make iso > iso.log 2>&1; then
    echo "✅ ISO created successfully"
else
    echo "❌ ISO creation failed! Check iso.log"
    cat iso.log
    exit 1
fi

echo ""
echo "=========================================="
echo "✅ BUILD COMPLETE!"
echo "=========================================="
echo ""
echo "ISO file: /workspaces/unix-v6/kernel/unix_v6.iso"
echo ""
echo "To boot, run:"
echo "  qemu-system-i386 -cdrom /workspaces/unix-v6/kernel/unix_v6.iso"
echo ""
echo "System should now boot to shell prompt without Trap #13 crash!"
echo ""
