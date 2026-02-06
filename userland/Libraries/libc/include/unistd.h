/* unistd.h - POSIX system calls and constants */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/types.h>

/* Process control */
pid_t fork(void);
int exec(const char *filename, char *const argv[]);
int execv(const char *filename, char *const argv[]);
int execve(const char *filename, char *const argv[], char *const envp[]);
void exit(int status) __attribute__((noreturn));
void _exit(int status) __attribute__((noreturn));
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t getpid(void);
pid_t getppid(void);

/* File operations */
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int fd[2]);
int access(const char *pathname, int mode);
int isatty(int fd);
int fsync(int fd);
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);

/* File metadata */

int link(const char *oldpath, const char *newpath);
int unlink(const char *path);
int rename(const char *oldpath, const char *newpath);

int chown(const char *path, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);


/* Directory operations */

int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);

/* User/Group IDs */
uid_t getuid(void);
gid_t getgid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);

/* Process groups */
pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
pid_t getpgrp(void);
pid_t getsid(pid_t pid);
pid_t setsid(void);

/* Signals */
int kill(pid_t pid, int sig);
int sigaction(int sig, const void *act, void *oact);
int sigprocmask(int how, unsigned long *set, unsigned long *oset);
int sigsuspend(unsigned long *mask);
int sigpending(unsigned long *set);
int sigreturn(void);
int alarm(unsigned int seconds);
int pause(void);

/* Time */
time_t time(time_t *tloc);
int gettimeofday(void *tv, void *tz);
int sleep(unsigned int seconds);
int nanosleep(const void *rqtp, void *rmtp);
int utime(const char *path, void *times);
int utimes(const char *filename, const void *times);

/* Memory */
int brk(void *addr);
void *sbrk(intptr_t incr);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t len, int prot);

/* I/O control */
int ioctl(int fd, unsigned long request, void *argp);
int select(int nfds, void *readfds, void *writefds, void *exceptfds, void *timeout);
int poll(void *fds, unsigned int nfds, int timeout);

/* Terminal control */
int tcgetattr(int fd, void *termios_p);
int tcsetattr(int fd, int optional_actions, const void *termios_p);
int tcflush(int fd, int queue_selector);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgid);

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
