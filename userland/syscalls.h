#ifndef SYSCALLS_H
#define SYSCALLS_H

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

extern int errno;

int fork(void);
int exec(const char *filename, char *const argv[]);
void exit(int status);
int wait(int *status);
int waitpid(int pid, int *status, int options);
int isatty(int fd);
int sigpending(unsigned long *set);
int sigreturn(void);
int mprotect(void *addr, unsigned long len, int prot);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
int open(const char *path, int mode);
int close(int fd);
int chdir(const char *path);
int getpid(void);
int getuid(void);
int getgid(void);
int setuid(int uid);
int setgid(int gid);
char *getcwd(char *buf, int size);
void _exit(int status);
int truncate(const char *path, int length);
int ftruncate(int fd, int length);
int fsync(int fd);
int utime(const char *path, void *times);
int pipe(int fd[2]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int stat(const char *path, void *buf);
int fstat(int fd, void *buf);
int lstat(const char *path, void *buf);
int link(const char *oldpath, const char *newpath);
int unlink(const char *path);
int rename(const char *oldpath, const char *newpath);
int mkdir(const char *path, int mode);
int rmdir(const char *path);
int chmod(const char *path, int mode);
int umask(int mask);
int brk(void *addr);
void *sbrk(int incr);
int sigaction(int sig, const void *act, void *oact);
int sigprocmask(int how, unsigned long *set, unsigned long *oset);
int sigsuspend(unsigned long *mask);
int alarm(int seconds);
int pause(void);
long time(long *tloc);
int gettimeofday(void *tv, void *tz);
int sleep(int seconds);
int nanosleep(const void *rqtp, void *rmtp);
int tcgetattr(int fd, void *termios_p);
int tcsetattr(int fd, int optional_actions, const void *termios_p);
int tcflush(int fd, int queue_selector);
int ioctl(int fd, unsigned long request, void *argp);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, int pgid);
int select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);
int poll(void *fds, unsigned int nfds, int timeout);
int getpgid(int pid);
int setpgid(int pid, int pgid);
int getsid(int pid);
int setsid(void);
void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset);
int munmap(void *addr, unsigned long length);
int getrlimit(int resource, void *rlim);
int setrlimit(int resource, const void *rlim);
int getrusage(int who, void *usage);
int chown(const char *path, int owner, int group);
int lchown(const char *path, int owner, int group);
int fchown(int fd, int owner, int group);
int fchmod(int fd, int mode);
int utimes(const char *filename, const void *times);
int fcntl(int fd, int cmd, long arg);
int access(const char *pathname, int mode);

#endif
