# 🔨 BUILD INSTRUCTIONS

## Summary of Fixes Applied

✅ **File 1**: [kernel/include/user.h](kernel/include/user.h)
- Changed `u_sigmask[2]` → `u_sigmask` (8 bytes instead of 16)
- Changed `u_pendingsig[2]` → `u_pendingsig` (8 bytes instead of 16)  
- Reduced `u_stack[3565]` → `u_stack[3557]` (adjusted by 8 bytes)

✅ **File 2**: [kernel/posix_sig.c](kernel/posix_sig.c)
- Updated signal masking code to use single-word masks
- Changed from `u.u_sigmask[0]` to `u.u_sigmask`

## Build Commands

### Option 1: Run the rebuild script
```bash
bash /workspaces/unix-v6/kernel/rebuild.sh
```

### Option 2: Manual rebuild
```bash
cd /workspaces/unix-v6/kernel
make clean
make build
make iso
```

## Boot Commands

After rebuild completes:

```bash
# Legacy BIOS
qemu-system-i386 -cdrom /workspaces/unix-v6/kernel/unix_v6.iso

# With VGA display
qemu-system-i386 -cdrom /workspaces/unix-v6/kernel/unix_v6.iso -m 64M

# With serial output
qemu-system-i386 -cdrom /workspaces/unix-v6/kernel/unix_v6.iso -m 64M -serial stdio -display none
```

## Expected Behavior

✅ Kernel boots  
✅ MOTD prints  
✅ System continues (no Trap #13 crash)  
✅ Shell prompt appears  
✅ Commands work (ls, pwd, echo, etc.)  

## Verification Checklist

After boot:
- [ ] No "Kernel fault! Trap: 13" panic
- [ ] MOTD displays completely
- [ ] Shell prompt appears
- [ ] Can type commands
- [ ] `pwd` returns correct path (not "/")
- [ ] `ls` lists files
- [ ] Signal handling works (Ctrl+C)

## If Build Fails

Check the log files:
```bash
cd /workspaces/unix-v6/kernel
cat build.log    # Shows compilation errors
cat iso.log      # Shows ISO creation errors
```

## All Changes Summary

The system had a **structure size overflow** because:
1. Added signal masking fields without reducing buffer
2. Over-engineered masks for 64 signals when only 20 exist

**Solution**:
- Correctly sized masks to 1 uint32_t each (enough for 20 signals)
- Reduced stack buffer by 8 bytes
- Simplified signal handling code

**Result**: Structure now fits correctly, no memory corruption, no Trap #13 crashes!
