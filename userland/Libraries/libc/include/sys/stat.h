/* sys/stat.h - File status structure */

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat {
    dev_t   st_dev;         /* 0: Device */
    ino_t   st_ino;         /* 2: Inode number */
    mode_t  st_mode;        /* 4: Mode */
    int8_t  st_nlink;       /* 6: Link count */
    uint8_t _pad1;          /* 7: Padding */
    uid_t   st_uid;         /* 8: User ID */
    gid_t   st_gid;         /* 10: Group ID */
    dev_t   st_rdev;        /* 12: Real Device */
    uint16_t _pad2;         /* 14: Padding */
    off_t   st_size;        /* 16: Size */
    time_t  st_atime;       /* 20: Access time */
    time_t  st_mtime;       /* 24: Modification time */
    time_t  st_ctime;       /* 28: Status change time */
};

/* File type flags (st_mode) */
#define S_IFMT   060000
#define S_IFDIR  040000
#define S_IFCHR  020000
#define S_IFBLK  060000
#define S_IFREG  000000
#define S_IFIFO  010000

/* Permissions */
#define S_ISUID  004000
#define S_ISGID  002000
#define S_ISVTX  001000
#define S_IREAD  000400
#define S_IWRITE 000200
#define S_IEXEC  000100

/* Helper macros */
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)

/* System calls */
int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
int mkdir(const char *path, mode_t mode);
int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
mode_t umask(mode_t mask);

#endif /* _SYS_STAT_H */
