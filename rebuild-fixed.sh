#!/bin/bash
# Fast rebuild script - user structure size fix
set -e

echo "🔨 Building Unix V6 x86 kernel (user structure size fix)..."
cd /workspaces/unix-v6/kernel

# Clean and rebuild
make clean > /dev/null 2>&1 || true
echo "✅ Cleaned old build"

make build > build.log 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Kernel compiled successfully"
else
    echo "❌ Kernel compilation failed"
    tail -20 build.log
    exit 1
fi

# Create ISO
make iso > iso.log 2>&1
if [ $? -eq 0 ]; then
    echo "✅ ISO created successfully"
else
    echo "❌ ISO creation failed"
    tail -20 iso.log
    exit 1
fi

echo ""
echo "=========================================="
echo "✅ BUILD COMPLETE"
echo "=========================================="
echo ""
echo "ISO location: /workspaces/unix-v6/kernel/unix_v6.iso"
echo ""
echo "To test, run:"
echo "  qemu-system-i386 -cdrom /workspaces/unix-v6/kernel/unix_v6.iso"
echo ""
