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

#endif
