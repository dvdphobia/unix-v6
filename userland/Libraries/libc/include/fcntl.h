/* fcntl.h - File control options */
#ifndef _FCNTL_H
#define _FCNTL_H

/* File access modes for open() */
#define O_RDONLY    0x0000  /* Open for reading only */
#define O_WRONLY    0x0001  /* Open for writing only */
#define O_RDWR      0x0002  /* Open for reading and writing */

/* File creation and status flags */
#define O_CREAT     0x0100  /* Create file if it doesn't exist */
#define O_EXCL      0x0200  /* Exclusive use flag */
#define O_NOCTTY    0x0400  /* Don't assign controlling terminal */
#define O_TRUNC     0x0800  /* Truncate file to zero length */
#define O_APPEND    0x1000  /* Append mode */
#define O_NONBLOCK  0x2000  /* Non-blocking mode */
#define O_SYNC      0x4000  /* Synchronous writes */

/* fcntl() commands */
#define F_DUPFD     0   /* Duplicate file descriptor */
#define F_GETFD     1   /* Get file descriptor flags */
#define F_SETFD     2   /* Set file descriptor flags */
#define F_GETFL     3   /* Get file status flags */
#define F_SETFL     4   /* Set file status flags */
#define F_GETLK     5   /* Get record locking information */
#define F_SETLK     6   /* Set record locking information */
#define F_SETLKW    7   /* Set record locking info; wait if blocked */

/* File descriptor flags (F_GETFD, F_SETFD) */
#define FD_CLOEXEC  1   /* Close on exec */

/* Function declarations */
int open(const char *pathname, int flags, ...);
int creat(const char *pathname, int mode);
int fcntl(int fd, int cmd, ...);

#endif /* _FCNTL_H */
