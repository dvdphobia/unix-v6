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
    { 0, nosys },           /* 49 = x */
    { 0, nosys },           /* 50 = x */
    { 0, nosys },           /* 51 = x */
    { 0, nosys },           /* 52 = x */
    { 0, nosys },           /* 53 = x */
    { 0, nosys },           /* 54 = x */
    { 0, nosys },           /* 55 = x */
    { 0, nosys },           /* 56 = x */
    { 0, nosys },           /* 57 = x */
    { 0, nosys },           /* 58 = x */
    { 0, nosys },           /* 59 = x */
    { 0, nosys },           /* 60 = x */
    { 0, nosys },           /* 61 = x */
    { 0, nosys },           /* 62 = x */
    { 0, nosys },           /* 63 = x */
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
