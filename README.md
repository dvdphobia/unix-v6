# Unix V6 x86 Port

A port of the historic Unix Sixth Edition kernel to the x86 architecture.

## Overview

This project is a modern port of the legendary Unix V6 operating system, originally
developed at Bell Labs in 1975 for the PDP-11 minicomputer. The kernel has been
ported to run on x86 (i386) hardware.

## Directory Structure

The project follows a macOS/BSD-style directory layout:

```
unix-v6/
├── bin/                    # User commands and utilities
├── sbin/                   # System binaries
├── etc/                    # System configuration files
│   ├── passwd             # User database
│   ├── group              # Group database
│   ├── rc                 # System startup script
│   ├── ttys               # Terminal configuration
│   └── motd               # Message of the day
├── kernel/                 # Kernel source code
│   ├── include/           # Kernel headers
│   ├── drivers/           # Device drivers
│   │   ├── block/        # Block device drivers
│   │   └── char/         # Character device drivers
│   ├── fs/               # File system code (alternate)
│   ├── core/             # Core kernel functions (alternate)
│   └── sched/            # Scheduler code
├── lib/                    # System libraries
│   └── libsystem/        # Core system library
├── include/                # Public system headers
│   └── unix/             # Unix headers
├── arch/                   # Architecture-specific code
│   └── x86/              # x86 port
├── tools/                  # Build tools and scripts
├── var/                    # Variable data
│   ├── log/              # System logs
│   └── run/              # Runtime data
├── dev/                    # Device files (placeholder)
├── tmp/                    # Temporary files
└── docs/                   # Documentation
```

## Building

### Prerequisites

- Cross-compiler: `i686-elf-gcc`
- Assembler: `i686-elf-as`
- GRUB: `grub-mkrescue`
- QEMU: `qemu-system-i386`

### Quick Build

```bash
make           # Build userland + kernel
make iso       # Create bootable ISO
make run       # Run in QEMU (serial)
```

See `docs/GETTING_STARTED.md` for the full build and extension guide.

### Userspace Programs

User programs live in `userland/bin` and are compiled into flat binaries.
The ramdisk is populated from `userland/ramdisk.manifest` during the build.
Example apps include `hello` and `netdemo` (network stub).

## Shell Commands

The userland shell supports:

| Command | Description |
|---------|-------------|
| `ls [path]` | List directory contents |
| `cd <dir>` | Change directory |
| `pwd` | Print working directory |
| `ps` | List processes |
| `echo <text>` | Echo text |
| `uname` | System information |
| `clear` | Clear screen |
| `help` | Show help |

## Troubleshooting

### TinyCC GPF Error

If `tcc --help` crashes with `TRAP: 13` (General Protection Fault):

**Quick Fix:**
```bash
# Rebuild TinyCC with updated flags
bash scripts/build_tinycc.sh
cd kernel && make iso
```

**Documentation:**
- [TINYCC_QUICKFIX.md](TINYCC_QUICKFIX.md) - Quick fix guide
- [TINYCC_FIX.md](TINYCC_FIX.md) - Comprehensive technical explanation
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - General troubleshooting

**Diagnostic Tool:**
```bash
bash check_tinycc.sh
```

### Other Issues

- **Build failures:** See [BUILD_AND_TEST_GUIDE.md](BUILD_AND_TEST_GUIDE.md)
- **Kernel errors:** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **General Protection Faults:** See [GPF_FIX_SUMMARY.md](GPF_FIX_SUMMARY.md)

## Historical References

- [Commentary on the Sixth Edition UNIX Operating System](http://www.lemis.com/grog/Documentation/Lions/) (Lions' Book)
- [The Unix Tree (Minnie's Home Page)](http://minnie.tuhs.org/cgi-bin/utree.pl)
- [Original V6 Source](http://www.tom-yam.or.jp/2238/src)

## License

See [LICENSE.txt](LICENSE.txt)
