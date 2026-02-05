/* errno.c - Error handling functions */

#include <stddef.h>

extern int errno;
extern int write(int fd, const void *buf, int count);
extern size_t strlen(const char *s);

/* Error messages */
static const char *error_messages[] = {
    "Success",                          /* 0 */
    "Operation not permitted",          /* 1 EPERM */
    "No such file or directory",        /* 2 ENOENT */
    "No such process",                  /* 3 ESRCH */
    "Interrupted system call",          /* 4 EINTR */
    "Input/output error",               /* 5 EIO */
    "No such device or address",        /* 6 ENXIO */
    "Argument list too long",           /* 7 E2BIG */
    "Exec format error",                /* 8 ENOEXEC */
    "Bad file descriptor",              /* 9 EBADF */
    "No child processes",               /* 10 ECHILD */
    "Resource temporarily unavailable", /* 11 EAGAIN */
    "Cannot allocate memory",           /* 12 ENOMEM */
    "Permission denied",                /* 13 EACCES */
    "Bad address",                      /* 14 EFAULT */
    "Block device required",            /* 15 ENOTBLK */
    "Device or resource busy",          /* 16 EBUSY */
    "File exists",                      /* 17 EEXIST */
    "Invalid cross-device link",        /* 18 EXDEV */
    "No such device",                   /* 19 ENODEV */
    "Not a directory",                  /* 20 ENOTDIR */
    "Is a directory",                   /* 21 EISDIR */
    "Invalid argument",                 /* 22 EINVAL */
    "Too many open files in system",    /* 23 ENFILE */
    "Too many open files",              /* 24 EMFILE */
    "Inappropriate ioctl for device",   /* 25 ENOTTY */
    "Text file busy",                   /* 26 ETXTBSY */
    "File too large",                   /* 27 EFBIG */
    "No space left on device",          /* 28 ENOSPC */
    "Illegal seek",                     /* 29 ESPIPE */
    "Read-only file system",            /* 30 EROFS */
    "Too many links",                   /* 31 EMLINK */
    "Broken pipe",                      /* 32 EPIPE */
    "Numerical argument out of domain", /* 33 EDOM */
    "Numerical result out of range",    /* 34 ERANGE */
};

#define NUM_ERRORS (sizeof(error_messages) / sizeof(error_messages[0]))

char *strerror(int errnum) {
    if (errnum < 0 || errnum >= (int)NUM_ERRORS)
        return "Unknown error";
    
    return (char *)error_messages[errnum];
}

void perror(const char *s) {
    if (s && *s) {
        write(2, s, strlen(s));
        write(2, ": ", 2);
    }
    
    const char *msg = strerror(errno);
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
}

/* Error number constants (for reference) */
#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define ENOTBLK         15
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define ETXTBSY         26
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define ERANGE          34
