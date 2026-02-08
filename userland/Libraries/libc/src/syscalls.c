/* syscalls.c */

#include <stddef.h>
#include <sys/syscall.h>

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

static long syscall5(long num, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    unsigned char err;
    register long r_si asm("esi") = a4;
    register long r_di asm("edi") = a5;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num), "b" (a1), "c" (a2), "d" (a3), "r" (r_si), "r" (r_di)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

static long syscall6(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    unsigned char err;
    register long r_si asm("esi") = a4;
    register long r_di asm("edi") = a5;
    /* Push 6th argument on stack, then do syscall */
    asm volatile ("pushl %3; int $0x80; addl $4, %%esp; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (num), "g" (a6), "b" (a1), "c" (a2), "d" (a3), "r" (r_si), "r" (r_di)
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

int execv(const char *filename, char *const argv[]) {
    return exec(filename, argv);
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    (void)envp; /* Ignore environment for now */
    return exec(filename, argv);
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

int waitpid(int pid, int *status, int options) {
    return (int)syscall3(SYS_WAITPID, pid, (long)status, options);
}

int isatty(int fd) {
    return (int)syscall1(SYS_ISATTY, fd);
}

int sigpending(unsigned long *set) {
    return (int)syscall1(SYS_SIGPENDING, (long)set);
}

int sigreturn(void) {
    return (int)syscall0(SYS_SIGRETURN);
}

int mprotect(void *addr, unsigned long len, int prot) {
    return (int)syscall3(SYS_MPROTECT, (long)addr, len, prot);
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

char *getcwd(char *buf, int size) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_GETCWD), "b" (buf), "c" (size)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return NULL;
    }
    return (char *)ret;
}

void _exit(int status) {
    (void)syscall1(SYS_EXIT2, status);
    while(1);
}

int truncate(const char *path, int length) {
    return (int)syscall2(SYS_TRUNCATE, (long)path, length);
}

int ftruncate(int fd, int length) {
    return (int)syscall2(SYS_FTRUNCATE, fd, length);
}

int fsync(int fd) {
    return (int)syscall1(SYS_FSYNC, fd);
}

int utime(const char *path, void *times) {
    return (int)syscall2(SYS_UTIME, (long)path, (long)times);
}
int pipe(int fd[2]) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_PIPE), "b" (fd)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return 0;
}

int dup(int oldfd) {
    return (int)syscall1(SYS_DUP, oldfd);
}

int dup2(int oldfd, int newfd) {
    return (int)syscall2(SYS_DUP2, oldfd, newfd);
}

int stat(const char *path, void *buf) {
    return (int)syscall2(SYS_STAT, (long)path, (long)buf);
}

int fstat(int fd, void *buf) {
    return (int)syscall2(SYS_FSTAT, fd, (long)buf);
}

int lstat(const char *path, void *buf) {
    return (int)syscall2(SYS_LSTAT, (long)path, (long)buf);
}

int link(const char *oldpath, const char *newpath) {
    return (int)syscall2(SYS_LINK, (long)oldpath, (long)newpath);
}

int unlink(const char *path) {
    return (int)syscall1(SYS_UNLINK, (long)path);
}

int rename(const char *oldpath, const char *newpath) {
    return (int)syscall2(SYS_RENAME, (long)oldpath, (long)newpath);
}

int mkdir(const char *path, int mode) {
    return (int)syscall2(SYS_MKDIR, (long)path, mode);
}

int rmdir(const char *path) {
    return (int)syscall1(SYS_RMDIR, (long)path);
}

int chmod(const char *path, int mode) {
    return (int)syscall2(SYS_CHMOD, (long)path, mode);
}

int umask(int mask) {
    return (int)syscall1(SYS_UMASK, mask);
}

int brk(void *addr) {
    return (int)syscall1(SYS_BRK, (long)addr);
}

void *sbrk(int incr) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_SBRK), "b" (incr)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return (void *)-1;
    }
    return (void *)ret;
}

int sigaction(int sig, const void *act, void *oact) {
    return (int)syscall3(SYS_SIGACTION, sig, (long)act, (long)oact);
}

int sigprocmask(int how, unsigned long *set, unsigned long *oset) {
    long ret;
    unsigned char err;
    long set_val = set ? *set : 0;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_SIGPROCMASK), "b" (how), "c" (set_val), "d" (oset)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    if (oset) {
        *oset = ret;
    }
    return 0;
}

int sigsuspend(unsigned long *mask) {
    long set_val = mask ? *mask : 0;
    return (int)syscall1(SYS_SIGSUSPEND, set_val);
}

int alarm(int seconds) {
    return (int)syscall1(SYS_ALARM, seconds);
}

int pause(void) {
    return (int)syscall0(SYS_PAUSE);
}

long time(long *tloc) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_TIME), "b" (tloc)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

int gettimeofday(void *tv, void *tz) {
    return (int)syscall2(SYS_GETTIMEOFDAY, (long)tv, (long)tz);
}

int sleep(int seconds) {
    return (int)syscall1(SYS_SLEEP, seconds);
}

int nanosleep(const void *rqtp, void *rmtp) {
    return (int)syscall2(SYS_NANOSLEEP, (long)rqtp, (long)rmtp);
}

int tcgetattr(int fd, void *termios_p) {
    return (int)syscall2(SYS_TCGETATTR, fd, (long)termios_p);
}

int tcsetattr(int fd, int optional_actions, const void *termios_p) {
    return (int)syscall3(SYS_TCSETATTR, fd, optional_actions, (long)termios_p);
}

int tcflush(int fd, int queue_selector) {
    return (int)syscall2(SYS_TCFLUSH, fd, queue_selector);
}

int ioctl(int fd, unsigned long request, void *argp) {
    return (int)syscall3(SYS_IOCTL, fd, request, (long)argp);
}

int tcgetpgrp(int fd) {
    int pgid = -1;
    if (ioctl(fd, 0x540F, &pgid) < 0) {
        return -1;
    }
    return pgid;
}

int tcsetpgrp(int fd, int pgid) {
    return ioctl(fd, 0x5410, &pgid);
}

int select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout) {
    return (int)syscall5(SYS_SELECT, nfds, (long)readfds, (long)writefds, (long)exceptfds, (long)timeout);
}

int poll(void *fds, unsigned int nfds, int timeout) {
    return (int)syscall3(SYS_POLL, (long)fds, nfds, timeout);
}

int getpgid(int pid) {
    return (int)syscall1(SYS_GETPGID, pid);
}

int setpgid(int pid, int pgid) {
    return (int)syscall2(SYS_SETPGID, pid, pgid);
}

int getsid(int pid) {
    return (int)syscall1(SYS_GETSID, pid);
}

int setsid(void) {
    return (int)syscall0(SYS_SETSID);
}

void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset) {
    return (void *)syscall6(SYS_MMAP, (long)addr, length, prot, flags, fd, offset);
}

int munmap(void *addr, unsigned long length) {
    return (int)syscall2(SYS_MUNMAP, (long)addr, length);
}

int getrlimit(int resource, void *rlim) {
    return (int)syscall2(SYS_GETRLIMIT, resource, (long)rlim);
}

int setrlimit(int resource, const void *rlim) {
    return (int)syscall2(SYS_SETRLIMIT, resource, (long)rlim);
}

int getrusage(int who, void *usage) {
    return (int)syscall2(SYS_GETRUSAGE, who, (long)usage);
}

int chown(const char *path, int owner, int group) {
    return (int)syscall3(SYS_CHOWN, (long)path, owner, group);
}

int lchown(const char *path, int owner, int group) {
    return (int)syscall3(SYS_LCHOWN, (long)path, owner, group);
}

int fchown(int fd, int owner, int group) {
    return (int)syscall3(SYS_FCHOWN, fd, owner, group);
}

int fchmod(int fd, int mode) {
    return (int)syscall2(SYS_FCHMOD, fd, mode);
}

int utimes(const char *filename, const void *times) {
    return (int)syscall2(SYS_UTIMES, (long)filename, (long)times);
}

int fcntl(int fd, int cmd, long arg) {
    return (int)syscall3(SYS_FCNTL, fd, cmd, arg);
}

int access(const char *pathname, int mode) {
    return (int)syscall2(SYS_ACCESS, (long)pathname, mode);
}

long lseek(int fd, long offset, int whence) {
    long ret;
    unsigned char err;
    asm volatile ("int $0x80; setc %1"
                  : "=a" (ret), "=q" (err)
                  : "0" (SYS_LSEEK), "b" (fd), "c" (offset), "d" (whence)
                  : "cc", "memory");
    if (err) {
        errno = (int)ret;
        return -1;
    }
    return ret;
}

int creat(const char *pathname, int mode) {
    return (int)syscall2(SYS_CREAT, (long)pathname, mode);
}

int kill(int pid, int sig) {
    return (int)syscall2(SYS_KILL, pid, sig);
}

int getppid(void) {
    return (int)syscall0(SYS_GETPPID);
}


int getpgrp(void) {
    return (int)syscall0(SYS_GETPGRP);
}

int getfbinfo(void *info) {
    return (int)syscall1(SYS_GETFBINFO, (long)info);
}
