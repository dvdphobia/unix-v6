# Starter Kit

Use these templates to add new user programs or drivers.

## Add a user program
1) Copy `kit/user_program.c` to `userland/bin/<name>.c`.
2) Add `<name>` to `userland/Makefile`.
3) Add an entry to `userland/ramdisk.manifest`:
   `0100755 /bin/<name> build/userland/<name>.bin`
4) Rebuild with `make` at the repo root.

## Add a driver
1) Copy `kit/driver_stub.c` into `kernel/drivers/<type>/<name>.c`.
2) Add the file to `KERNEL_SRCS` or `DRIVER_SRCS` in `kernel/Makefile`.
3) Rebuild with `make`.
