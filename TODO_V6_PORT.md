# Remaining Tasks for Full Unix V6 x86 Port

This document outlines the critical components that are currently missing, simulated, or stubbed out in this system. To achieve a fully functional Unix V6 compatible system, these areas need to be implemented.

## 1. Memory Management & Protection (Critical)
The current memory model is flat and unprotected.
- [x] **Implement `estabur()`**: Implemented limit checking for flat memory model.
- [x] **Virtual Memory/Isolation**: Implemented GDT-based Base relocation in `swtch` and software-based relocation in `fubyte`/`subyte`.
- [x] **Swapping**: Implemented `malloc`/`mfree` based memory management (without disk swapping).

## 2. Process Execution & Userspace
The system currently runs an "embedded" shell within the kernel.
- [x] **Real Process 1**: `kernel/main.c` uses `newproc` and `icode` to exec `/etc/init`.
- [x] **User Mode Shell**: Created `userspace/sh.c`, compiled to `/bin/sh` on ramdisk.
- [x] **Safe `exec()`**: `exec()` uses `expand()` and `estabur()` for memory safety.

## 3. Drivers & Hardware
The system is currently ephemeral and lacks persistent I/O.
- [x] **Disk Driver**: Minimal ATA PIO driver for QEMU primary master (`drivers/block/ide.c`).
- [x] **Terminal Driver**: Blocking console input implemented in `drivers/char/console.c` (basic line reads).

## 4. System Calls & Kernel Features
Several kernel subsystems are simplified.
- [x] **Signal Delivery**: Basic signal infrastructure exists.
- [x] **Timekeeping**: `clock.c` exists.
- [x] **Mount/Unmount**: Basic `smount`/`sumount` implemented using the mount table.
