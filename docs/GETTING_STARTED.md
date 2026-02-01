# Getting Started

## Build and Run

```bash
make
make run
```

If you do not have the cross toolchain, run:

```bash
make toolchain
```

## Add a User Program

1) Copy `kit/user_program.c` to `userland/bin/<name>.c`.
2) Add `<name>` to `userland/Makefile`.
3) Add an entry to `userland/ramdisk.manifest`:

```
0100755 /bin/<name> build/userland/<name>.bin
```

4) Rebuild with `make`.

## Troubleshooting

- If `/bin/sh` fails to exec, confirm `userland/ramdisk.manifest` matches your binaries.
- If the build cannot find `i686-elf-gcc`, run `make toolchain` or add it to your PATH.
