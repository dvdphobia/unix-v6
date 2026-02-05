# Unix V6 POSIX C Library (libc)

A complete POSIX-compliant C library implementation for the Unix V6 operating system.

## Structure

```
libc/
├── crt/              ← C Runtime (portable across architectures)
│   ├── crt0.c        ← Full startup with ctors/dtors
│   ├── crt1.c        ← Minimal startup
│   ├── crti.c        ← Init prologue
│   ├── crtn.c        ← Init epilogue
│   ├── crtbegin.c    ← Constructor support
│   ├── crtend.c      ← Destructor support
│   ├── Makefile
│   └── README.md
├── string.c/h        ← String functions
├── stdlib.c/h        ← Memory allocation, conversions
├── stdio.c/h         ← File I/O, printf
├── ctype.c/h         ← Character classification
├── dirent.c/h        ← Directory operations
├── errno.c/h         ← Error handling
├── unistd.h          ← System call declarations
├── Makefile
└── README.md         ← This file
```

## Contents

### Core Libraries

- **string.c/h** - String manipulation functions
  - `strlen`, `strcpy`, `strncpy`, `strcat`, `strncat`
  - `strcmp`, `strncmp`, `strchr`, `strrchr`, `strstr`
  - `memset`, `memcpy`, `memmove`, `memcmp`, `memchr`
  - BSD functions: `bzero`, `bcopy`, `bcmp`

- **stdlib.c/h** - Standard library functions
  - Memory: `malloc`, `calloc`, `realloc`, `free`
  - Conversion: `atoi`, `atol`
  - Math: `abs`, `labs`
  - Process: `exit`, `abort`
  - Environment: `getenv`, `setenv`, `unsetenv`

- **stdio.c/h** - Standard I/O functions
  - File operations: `fopen`, `fclose`, `fflush`
  - Character I/O: `fgetc`, `fputc`, `getchar`, `putchar`, `ungetc`
  - Line I/O: `fgets`, `fputs`, `puts`
  - Block I/O: `fread`, `fwrite`
  - Positioning: `fseek`, `ftell`
  - Formatted output: `printf`, `fprintf`, `sprintf`, `snprintf`
  - Error handling: `feof`, `ferror`, `clearerr`

- **ctype.c/h** - Character classification
  - Classification: `isalpha`, `isdigit`, `isalnum`, `isspace`, `isupper`, `islower`
  - `isprint`, `isgraph`, `iscntrl`, `isxdigit`, `ispunct`, `isascii`
  - Conversion: `toupper`, `tolower`, `toascii`

- **dirent.c/h** - Directory operations
  - `opendir`, `readdir`, `closedir`
  - `rewinddir`, `telldir`, `seekdir`

- **errno.c/h** - Error handling
  - `strerror` - Convert errno to string
  - `perror` - Print error message
  - All standard error constants (EPERM, ENOENT, etc.)

- **unistd.h** - POSIX system call declarations
  - Complete declarations for all syscalls
  - Constants for file operations, signals, etc.

## System Calls Provided

### Process Management
- `fork`, `exec`, `execv`, `execve`, `exit`, `_exit`
- `wait`, `waitpid`, `getpid`, `getppid`
- `kill`, `alarm`, `pause`, `sleep`

### File Operations
- `open`, `close`, `read`, `write`, `lseek`, `creat`
- `dup`, `dup2`, `pipe`, `access`, `isatty`
- `stat`, `fstat`, `lstat`, `link`, `unlink`, `rename`
- `chmod`, `fchmod`, `chown`, `lchown`, `fchown`
- `mkdir`, `rmdir`, `chdir`, `getcwd`
- `truncate`, `ftruncate`, `fsync`, `utime`, `utimes`

### User/Group
- `getuid`, `getgid`, `setuid`, `setgid`
- `getpgid`, `setpgid`, `getpgrp`, `getsid`, `setsid`

### Memory
- `brk`, `sbrk`, `mmap`, `munmap`, `mprotect`

### Signals
- `sigaction`, `sigprocmask`, `sigsuspend`, `sigpending`, `sigreturn`

### Terminal
- `tcgetattr`, `tcsetattr`, `tcflush`
- `tcgetpgrp`, `tcsetpgrp`, `ioctl`

### Advanced I/O
- `select`, `poll`, `fcntl`

### Resources
- `getrlimit`, `setrlimit`, `getrusage`
- `umask`

## Building

The libc is automatically built when you build userland programs:

```bash
cd userland
make          # Builds CRT + libc + all programs
```

Or build just the library:

```bash
cd userland/libc
make          # Builds libc.a and CRT objects
make crt      # Builds just CRT objects
make clean    # Clean all objects
```

## C Runtime (CRT)

The CRT is now **portable C code** that works across different architectures:
- x86 (i386/i686)
- x86-64 (amd64)
- ARM (32-bit & 64-bit)
- RISC-V
- Easy to add more!

See [crt/README.md](crt/README.md) for details on the portable CRT implementation.

## Usage in Programs

Include the appropriate headers:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("Hello, world!\n");
    
    char *mem = malloc(100);
    strcpy(mem, "Dynamic memory!");
    printf("%s\n", mem);
    free(mem);
    
    return 0;
}
```

## Implementation Notes

- **malloc**: Simple first-fit allocator using `sbrk`
- **printf**: Supports %d, %i, %u, %x, %X, %p, %s, %c format specifiers
- **stdio**: Buffered I/O with 1KB buffer (BUFSIZ)
- **File streams**: Maximum 20 open files (FOPEN_MAX)
- **Environment**: Simple 256-entry environment storage

## Completeness

This libc provides:
- ✅ All essential POSIX system call wrappers
- ✅ Complete string manipulation
- ✅ Dynamic memory allocation
- ✅ Formatted I/O (printf family)
- ✅ File I/O (fopen, fread, fwrite, etc.)
- ✅ Directory operations
- ✅ Character classification
- ✅ Error handling

Still TODO (for full POSIX.1):
- Regular expressions (regex.h)
- Math library (libm)
- Threading (pthread)
- Inter-process communication (shared memory, semaphores, message queues)
- Network sockets
- Advanced time functions (strftime, mktime, etc.)

## License

Part of the Unix V6 x86 port project.
