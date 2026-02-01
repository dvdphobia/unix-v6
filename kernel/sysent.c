/* sysent.c - Unix V6 x86 Port System Call Table
 * Ported from original V6 ken/sysent.c for PDP-11
 * Switch table for processing system calls
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/systm.h"
#include "include/user.h"

/* Forward declarations for system call handlers */
/* These are implemented in sys.c and sig.c */
int nullsys(void);
int nosys(void);
int rexit(void);
int fork(void);
int sysread(void);
int syswrite(void);
int sysopen(void);
int sysclose(void);
int syswait(void);
int creat(void);
int link(void);
int unlink(void);
int exec(void);
int chdir(void);
int gtime(void);
int mknod(void);
int chmod(void);
int chown(void);
int sbreak(void);
int stat(void);
int seek(void);
int getpid(void);
int smount(void);
int sumount(void);
int setuid(void);
int getuid(void);
int stime(void);
int ptrace(void);
int fstat(void);
int stty(void);
int gtty(void);
int nice(void);
int sslep(void);
int sync(void);
int kill(void);
int getswit(void);
int dup(void);
int syspipe(void);
int times(void);
int profil(void);
int setgid(void);
int getgid(void);
int ssig(void);
int getcwd(void);
int sys_exit(void);
int truncate(void);
int ftruncate(void);
int fsync(void);
int utime(void);
int sys_pipe(void);
int sys_dup(void);
int sys_dup2(void);
int sys_stat(void);
int sys_fstat(void);
int sys_lstat(void);
int sys_link(void);
int sys_unlink(void);
int sys_rename(void);
int sys_mkdir(void);
int sys_rmdir(void);
int sys_chmod(void);
int sys_umask(void);
int sys_brk(void);
int sys_sbrk(void);
int sys_sigaction(void);
int sys_sigprocmask(void);
int sys_sigsuspend(void);
int sys_alarm(void);
int sys_pause(void);
int sys_time(void);
int sys_gettimeofday(void);
int sys_sleep(void);
int sys_nanosleep(void);
int sys_tcgetattr(void);
int sys_tcsetattr(void);
int sys_tcflush(void);
int sys_ioctl(void);
int sys_select(void);
int sys_poll(void);
int sys_getpgid(void);
int sys_setpgid(void);
int sys_getsid(void);
int sys_setsid(void);
int sys_mmap(void);
int sys_munmap(void);
int sys_getrlimit(void);
int sys_setrlimit(void);
int sys_getrusage(void);
int sys_chown2(void);
int sys_lchown(void);
int sys_fchown(void);
int sys_fchmod(void);
int sys_utimes(void);
int sys_fcntl(void);
int sys_access(void);
int sys_isatty(void);
int sys_sigpending(void);
int sys_sigreturn(void);
int sys_mprotect(void);
int sys_waitpid(void);

/*
 * System call entry table
 * Each entry contains: { nargs, handler_function }
 *
 * Based on original V6 sysent[] table
 */
struct sysent {
    int8_t  sy_narg;            /* Number of arguments */
    int     (*sy_call)(void);   /* Handler function */
};

struct sysent sysent[] = {
    { 0, nullsys },         /*  0 = indir/nosys */
    { 0, rexit },           /*  1 = exit */
    { 0, fork },            /*  2 = fork */
    { 2, sysread },         /*  3 = read */
    { 2, syswrite },        /*  4 = write */
    { 2, sysopen },         /*  5 = open */
    { 0, sysclose },        /*  6 = close */
    { 0, syswait },         /*  7 = wait */
    { 2, creat },           /*  8 = creat */
    { 2, link },            /*  9 = link */
    { 1, unlink },          /* 10 = unlink */
    { 2, exec },            /* 11 = exec */
    { 1, chdir },           /* 12 = chdir */
    { 0, gtime },           /* 13 = time */
    { 3, mknod },           /* 14 = mknod */
    { 2, chmod },           /* 15 = chmod */
    { 2, chown },           /* 16 = chown */
    { 1, sbreak },          /* 17 = break */
    { 2, stat },            /* 18 = stat */
    { 2, seek },            /* 19 = seek */
    { 0, getpid },          /* 20 = getpid */
    { 3, smount },          /* 21 = mount */
    { 1, sumount },         /* 22 = umount */
    { 0, setuid },          /* 23 = setuid */
    { 0, getuid },          /* 24 = getuid */
    { 0, stime },           /* 25 = stime */
    { 3, ptrace },          /* 26 = ptrace */
    { 0, nosys },           /* 27 = x */
    { 1, fstat },           /* 28 = fstat */
    { 0, nosys },           /* 29 = x */
    { 1, nullsys },         /* 30 = smdate (inoperative) */
    { 1, stty },            /* 31 = stty */
    { 1, gtty },            /* 32 = gtty */
    { 0, nosys },           /* 33 = x */
    { 0, nice },            /* 34 = nice */
    { 0, sslep },           /* 35 = sleep */
    { 0, sync },            /* 36 = sync */
    { 1, kill },            /* 37 = kill */
    { 0, getswit },         /* 38 = switch */
    { 0, nosys },           /* 39 = x */
    { 0, nosys },           /* 40 = x */
    { 0, dup },             /* 41 = dup */
    { 0, syspipe },         /* 42 = pipe */
    { 1, times },           /* 43 = times */
    { 4, profil },          /* 44 = prof */
    { 0, nosys },           /* 45 = tiu */
    { 0, setgid },          /* 46 = setgid */
    { 0, getgid },          /* 47 = getgid */
    { 2, ssig },            /* 48 = sig */
    { 2, getcwd },          /* 49 = getcwd */
    { 1, sys_exit },        /* 50 = _exit */
    { 2, truncate },        /* 51 = truncate */
    { 2, ftruncate },       /* 52 = ftruncate */
    { 1, fsync },           /* 53 = fsync */
    { 2, utime },           /* 54 = utime */
    { 0, sys_pipe },        /* 55 = pipe */
    { 1, sys_dup },         /* 56 = dup */
    { 2, sys_dup2 },        /* 57 = dup2 */
    { 2, sys_stat },        /* 58 = stat */
    { 2, sys_fstat },       /* 59 = fstat */
    { 2, sys_lstat },       /* 60 = lstat */
    { 2, sys_link },        /* 61 = link */
    { 1, sys_unlink },      /* 62 = unlink */
    { 2, sys_rename },      /* 63 = rename */
    { 2, sys_mkdir },       /* 64 = mkdir */
    { 1, sys_rmdir },       /* 65 = rmdir */
    { 2, sys_chmod },       /* 66 = chmod */
    { 1, sys_umask },       /* 67 = umask */
    { 1, sys_brk },         /* 68 = brk */
    { 1, sys_sbrk },        /* 69 = sbrk */
    { 3, sys_sigaction },   /* 70 = sigaction */
    { 3, sys_sigprocmask }, /* 71 = sigprocmask */
    { 1, sys_sigsuspend },  /* 72 = sigsuspend */
    { 1, sys_alarm },       /* 73 = alarm */
    { 0, sys_pause },       /* 74 = pause */
    { 1, sys_time },        /* 75 = time */
    { 2, sys_gettimeofday },/* 76 = gettimeofday */
    { 1, sys_sleep },       /* 77 = sleep */
    { 1, sys_nanosleep },   /* 78 = nanosleep */
    { 2, sys_tcgetattr },   /* 79 = tcgetattr */
    { 3, sys_tcsetattr },   /* 80 = tcsetattr */
    { 2, sys_tcflush },     /* 81 = tcflush */
    { 3, sys_ioctl },       /* 82 = ioctl */
    { 5, sys_select },      /* 83 = select */
    { 3, sys_poll },        /* 84 = poll */
    { 1, sys_getpgid },     /* 85 = getpgid */
    { 2, sys_setpgid },     /* 86 = setpgid */
    { 1, sys_getsid },      /* 87 = getsid */
    { 0, sys_setsid },      /* 88 = setsid */
    { 6, sys_mmap },        /* 89 = mmap */
    { 2, sys_munmap },      /* 90 = munmap */
    { 2, sys_getrlimit },   /* 91 = getrlimit */
    { 2, sys_setrlimit },   /* 92 = setrlimit */
    { 2, sys_getrusage },   /* 93 = getrusage */
    { 3, sys_chown2 },      /* 94 = chown */
    { 3, sys_lchown },      /* 95 = lchown */
    { 3, sys_fchown },      /* 96 = fchown */
    { 2, sys_fchmod },      /* 97 = fchmod */
    { 2, sys_utimes },      /* 98 = utimes */
    { 3, sys_fcntl },       /* 99 = fcntl */
    { 2, sys_access },      /* 100 = access */
    { 3, sys_waitpid },     /* 101 = waitpid */
    { 1, sys_isatty },      /* 102 = isatty */
    { 1, sys_sigpending },  /* 103 = sigpending */
    { 0, sys_sigreturn },   /* 104 = sigreturn */
    { 3, sys_mprotect },    /* 105 = mprotect */
};

/* Number of system calls */
int nsysent = sizeof(sysent) / sizeof(struct sysent);

/*
 * nullsys - Null system call (do nothing)
 */
int nullsys(void) {
    return 0;
}

/*
 * nosys - Undefined system call
 */
int nosys(void) {
    extern struct user u;
    u.u_error = EINVAL;
    return -1;
}
