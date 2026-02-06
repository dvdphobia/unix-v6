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
- [x] **User Mode Shell**: Created `userland/sh.c`, compiled to `/bin/sh` on ramdisk.
- [x] **Safe `exec()`**: `exec()` uses `expand()` and `estabur()` for memory safety.

## 3. Drivers & Hardware
The system is currently ephemeral and lacks persistent I/O.
- [x] **Disk Driver**: Minimal ATA PIO driver for QEMU primary master (`drivers/block/ide.c`).
- [x] **Terminal Driver**: Blocking console input implemented in `drivers/char/console.c` (basic line reads).

## 3.1 POSIX Gaps Mapped for GCC (Priority Order)
This section maps missing POSIX pieces to the files they require, with focus on what GCC needs first.

### Tier 1 (GCC‑critical)
**Process & exec pipeline**
- [ ] `fork`/`exec`/`wait` completeness (exit status, argv/envp handling)
  - Files: [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/sysent.c](kernel/core/sysent.c), [userland/Libraries/libc/src/syscalls.c](userland/Libraries/libc/src/syscalls.c)
- [ ] `pipe`, `dup`, `dup2`, `close-on-exec`
  - Files: [kernel/core/pipe.c](kernel/core/pipe.c), [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/sysent.c](kernel/core/sysent.c)

**Files & metadata**
- [ ] `stat`, `fstat`, `lstat` (mode, size, times)
  - Files: [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/sysent.c](kernel/core/sysent.c), [kernel/fs/iget.c](kernel/fs/iget.c)
- [ ] `link`, `unlink`, `rename`, `mkdir`, `rmdir`, `chmod`, `umask`
  - Files: [kernel/core/sys.c](kernel/core/sys.c), [kernel/fs/nami.c](kernel/fs/nami.c), [kernel/fs/alloc.c](kernel/fs/alloc.c)

**Memory**
- [ ] `brk`/`sbrk` (user heap growth)
  - Files: [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/malloc.c](kernel/core/malloc.c), [userland/Libraries/libc/src/syscalls.c](userland/Libraries/libc/src/syscalls.c)

**Signals**
- [ ] `sigaction`, `sigprocmask`, `sigsuspend`, `alarm`, `pause`
  - Files: [kernel/core/sig.c](kernel/core/sig.c), [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/sysent.c](kernel/core/sysent.c)

**Time**
- [ ] `time`, `gettimeofday`, `sleep`/`nanosleep`
  - Files: [kernel/core/clock.c](kernel/core/clock.c), [kernel/core/sys.c](kernel/core/sys.c)

### Tier 2 (needed for a usable build environment)
**TTY/termios & ioctl**
- [ ] `tcgetattr`, `tcsetattr`, `tcflush`, `ioctl` basics
  - Files: [kernel/core/tty.c](kernel/core/tty.c), [kernel/drivers/char/tty.c](kernel/drivers/char/tty.c), [kernel/core/sys.c](kernel/core/sys.c)

**I/O multiplexing**
- [ ] `select`/`poll`
  - Files: [kernel/core/sys.c](kernel/core/sys.c), [kernel/core/pipe.c](kernel/core/pipe.c), [kernel/core/tty.c](kernel/core/tty.c)

**Users/groups**
- [ ] `getuid`, `getgid`, `setuid`, `setgid`, group/passwd lookups
  - Files: [kernel/core/sys.c](kernel/core/sys.c)

### Tier 3 (quality & completeness)
- [ ] `mmap`, `munmap` (optional if `sbrk` is stable)
- [ ] `getrlimit`, `setrlimit`, `getrusage`
- [ ] `chown`, `lchown`, `fchmod` variants
- [ ] `utimes` (higher‑resolution timestamps)
- [ ] `fcntl` (file flags, locking)

## 3.2 Toolchain prerequisites (beyond POSIX)
To run GCC natively you also need:
- [ ] Assembler + linker (binutils or LLVM toolchain)
- [ ] Working `make` and a shell with redirection and pipes

## 3.3 Recommended next step
Implement Tier‑1 items first for full POSIX compatibility.

## 4. System Calls & Kernel Features
Several kernel subsystems are simplified.
- [x] **Signal Delivery**: Basic signal infrastructure exists.
- [x] **Timekeeping**: `clock.c` exists.
- [x] **Mount/Unmount**: Basic `smount`/`sumount` implemented using the mount table.
