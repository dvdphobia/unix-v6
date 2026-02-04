/* unistd.h - POSIX system calls and constants */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

/* Process control */
int fork(void);
int exec(const char *filename, char *const argv[]);
int execv(const char *filename, char *const argv[]);
int execve(const char *filename, char *const argv[], char *const envp[]);
void exit(int status) __attribute__((noreturn));
void _exit(int status) __attribute__((noreturn));
int wait(int *status);
int waitpid(int pid, int *status, int options);
int getpid(void);
int getppid(void);

/* File operations */
int close(int fd);
int read(int fd, void *buf, int count);
int write(int fd, const void *buf, int count);
long lseek(int fd, long offset, int whence);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int fd[2]);
int access(const char *pathname, int mode);
int isatty(int fd);
int fsync(int fd);
int truncate(const char *path, int length);
int ftruncate(int fd, int length);

/* File metadata */
int stat(const char *path, void *buf);
int fstat(int fd, void *buf);
int lstat(const char *path, void *buf);
int link(const char *oldpath, const char *newpath);
int unlink(const char *path);
int rename(const char *oldpath, const char *newpath);
int chmod(const char *path, int mode);
int fchmod(int fd, int mode);
int chown(const char *path, int owner, int group);
int lchown(const char *path, int owner, int group);
int fchown(int fd, int owner, int group);
int umask(int mask);

/* Directory operations */
int mkdir(const char *path, int mode);
int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, int size);

/* User/Group IDs */
int getuid(void);
int getgid(void);
int setuid(int uid);
int setgid(int gid);

/* Process groups */
int getpgid(int pid);
int setpgid(int pid, int pgid);
int getpgrp(void);
int getsid(int pid);
int setsid(void);

/* Signals */
int kill(int pid, int sig);
int sigaction(int sig, const void *act, void *oact);
int sigprocmask(int how, unsigned long *set, unsigned long *oset);
int sigsuspend(unsigned long *mask);
int sigpending(unsigned long *set);
int sigreturn(void);
int alarm(int seconds);
int pause(void);

/* Time */
long time(long *tloc);
int gettimeofday(void *tv, void *tz);
int sleep(int seconds);
int nanosleep(const void *rqtp, void *rmtp);
int utime(const char *path, void *times);
int utimes(const char *filename, const void *times);

/* Memory */
int brk(void *addr);
void *sbrk(int incr);
void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset);
int munmap(void *addr, unsigned long length);
int mprotect(void *addr, unsigned long len, int prot);

/* I/O control */
int ioctl(int fd, unsigned long request, void *argp);
int select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);
int poll(void *fds, unsigned int nfds, int timeout);

/* Terminal control */
int tcgetattr(int fd, void *termios_p);
int tcsetattr(int fd, int optional_actions, const void *termios_p);
int tcflush(int fd, int queue_selector);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, int pgid);

/* Resource limits */
int getrlimit(int resource, void *rlim);
int setrlimit(int resource, const void *rlim);
int getrusage(int who, void *usage);

/* Constants */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* lseek whence values */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* access mode values */
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

/* NULL pointer */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* _UNISTD_H */
