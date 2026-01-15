/* syscalls.c */

/* Syscall numbers */
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_WAIT    7
#define SYS_EXEC    11
#define SYS_CHDIR   12
#define SYS_GETPID  20
#define SYS_SETUID  23
#define SYS_GETUID  24
#define SYS_SETGID  46
#define SYS_GETGID  47

int errno;

static long syscall0(long num) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

static long syscall1(long num, long a1) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num), "b" (a1)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

static long syscall2(long num, long a1, long a2) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num), "b" (a1), "c" (a2)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

static long syscall3(long num, long a1, long a2, long a3) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num), "b" (a1), "c" (a2), "d" (a3)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

int fork(void) {
    return (int)syscall0(SYS_FORK);
}

int exec(const char *filename, char *const argv[]) {
    return (int)syscall2(SYS_EXEC, (long)filename, (long)argv);
}

void exit(int status) {
    (void)syscall1(SYS_EXIT, status);
    while(1);
}

int wait(int *status) {
    long ret;
    long st;
    unsigned char err;
    asm volatile ("int $0x80; setc %2"
                  : "=a" (ret), "=c" (st), "=q" (err)
                  : "0" (SYS_WAIT)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    if (status) {
        *status = (int)st;
    }
    return (int)ret;
}

int write(int fd, const void *buf, int count) {
    return (int)syscall3(SYS_WRITE, fd, (long)buf, count);
}

int read(int fd, void *buf, int count) {
    return (int)syscall3(SYS_READ, fd, (long)buf, count);
}

int open(const char *path, int mode) {
    return (int)syscall2(SYS_OPEN, (long)path, mode);
}

int close(int fd) {
    return (int)syscall1(SYS_CLOSE, fd);
}

int chdir(const char *path) {
    return (int)syscall1(SYS_CHDIR, (long)path);
}

int getpid(void) {
    return (int)syscall0(SYS_GETPID);
}

int getuid(void) {
    return (int)syscall0(SYS_GETUID);
}

int getgid(void) {
    return (int)syscall0(SYS_GETGID);
}

int setuid(int uid) {
    return (int)syscall1(SYS_SETUID, uid);
}

int setgid(int gid) {
    return (int)syscall1(SYS_SETGID, gid);
}
