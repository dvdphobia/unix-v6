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
int getfbinfo(void);
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
int sys_enosys(void);
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
int sys_futex_time64(void);
int sys_rt_sigqueueinfo(void);
int sys_sched_getaffinity(void);
int sys_msgget(void);
int sys_msgctl(void);
int sys_msgsnd(void);
int sys_msgrcv(void);
int sys_semget(void);
int sys_semctl(void);
int sys_semop(void);
int sys_capget(void);
int sys_capset(void);
int sys_chroot(void);
int sys_clock_adjtime64(void);
int sys_shmget(void);
int sys_shmctl(void);
int sys_shmat(void);
int sys_shmdt(void);
int sys_copy_file_range(void);
int sys_epoll_create1(void);
int sys_epoll_ctl(void);
int sys_epoll_pwait(void);
int sys_getdents(void);
int sys_getitimer(void);
int sys_rt_sigprocmask(void);
int sys_set_tid_address(void);
int sys_eventfd2(void);
int sys_flock(void);
int sys_fanotify_init(void);
int sys_fanotify_mark(void);
int sys_fallocate(void);
int sys_rt_sigaction(void);
int sys_tkill(void);
int sys_exit_group(void);
int sys_fadvise(void);
int sys_openat(void);
int sys_getrandom(void);
int sys_inotify_init1(void);
int sys_inotify_add_watch(void);
int sys_inotify_rm_watch(void);
int sys_getpriority(void);
int sys_getresgid(void);
int sys_setitimer(void);
int sys_waitid(void);
int sys_syslog(void);
int sys_memfd_create(void);
int sys_mlock2(void);
int sys_membarrier(void);
int sys_init_module(void);
int sys_delete_module(void);
int sys_mount2(void);
int sys_umount2(void);
int sys_name_to_handle_at(void);
int sys_open_by_handle_at(void);
int sys_ok(void);

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
    { 1, getfbinfo },       /* 39 = getfbinfo */
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
    { 6, sys_futex_time64 },/* 106 = futex_time64 (stub) */
    { 3, sys_rt_sigqueueinfo }, /* 107 = rt_sigqueueinfo (stub) */
    { 3, sys_sched_getaffinity }, /* 108 = sched_getaffinity (stub) */
    { 2, sys_msgget },     /* 109 = msgget (stub) */
    { 3, sys_msgctl },     /* 110 = msgctl (stub) */
    { 4, sys_msgsnd },     /* 111 = msgsnd (stub) */
    { 5, sys_msgrcv },     /* 112 = msgrcv (stub) */
    { 3, sys_semget },     /* 113 = semget (stub) */
    { 3, sys_semctl },     /* 114 = semctl (stub) */
    { 3, sys_semop },      /* 115 = semop (stub) */
    { 2, sys_capget },     /* 116 = capget (stub) */
    { 2, sys_capset },     /* 117 = capset (stub) */
    { 1, sys_chroot },     /* 118 = chroot (stub) */
    { 2, sys_clock_adjtime64 }, /* 119 = clock_adjtime64 (stub) */
    { 3, sys_shmget },     /* 120 = shmget (stub) */
    { 3, sys_shmctl },     /* 121 = shmctl (stub) */
    { 3, sys_shmat },      /* 122 = shmat (stub) */
    { 1, sys_ok },         /* 123 = shmdt (no-op) */
    { 6, sys_copy_file_range }, /* 124 = copy_file_range (stub) */
    { 1, sys_epoll_create1 },   /* 125 = epoll_create1 (stub) */
    { 4, sys_epoll_ctl },       /* 126 = epoll_ctl (stub) */
    { 6, sys_epoll_pwait },     /* 127 = epoll_pwait (stub) */
    { 3, sys_getdents },        /* 128 = getdents (stub) */
    { 2, sys_getitimer },       /* 129 = getitimer (stub) */
    { 4, sys_rt_sigprocmask },  /* 130 = rt_sigprocmask (stub) */
    { 1, sys_set_tid_address }, /* 131 = set_tid_address (stub) */
    { 2, sys_eventfd2 },        /* 132 = eventfd2 (stub) */
    { 2, sys_flock },           /* 133 = flock (stub) */
    { 2, sys_fanotify_init },   /* 134 = fanotify_init (stub) */
    { 6, sys_fanotify_mark },   /* 135 = fanotify_mark (stub) */
    { 6, sys_fallocate },       /* 136 = fallocate (stub) */
    { 4, sys_rt_sigaction },    /* 137 = rt_sigaction (stub) */
    { 2, sys_tkill },           /* 138 = tkill (stub) */
    { 1, sys_exit_group },      /* 139 = exit_group (stub) */
    { 6, sys_fadvise },         /* 140 = fadvise (stub) */
    { 4, sys_openat },          /* 141 = openat (stub) */
    { 3, sys_getrandom },       /* 142 = getrandom (stub) */
    { 1, sys_inotify_init1 },   /* 143 = inotify_init1 (stub) */
    { 3, sys_inotify_add_watch }, /* 144 = inotify_add_watch (stub) */
    { 2, sys_inotify_rm_watch }, /* 145 = inotify_rm_watch (stub) */
    { 2, sys_getpriority },     /* 146 = getpriority (stub) */
    { 3, sys_getresgid },       /* 147 = getresgid (stub) */
    { 3, sys_setitimer },       /* 148 = setitimer (stub) */
    { 5, sys_waitid },          /* 149 = waitid (stub) */
    { 3, sys_syslog },          /* 150 = syslog (stub) */
    { 2, sys_memfd_create },    /* 151 = memfd_create (stub) */
    { 3, sys_mlock2 },          /* 152 = mlock2 (stub) */
    { 2, sys_membarrier },      /* 153 = membarrier (stub) */
    { 3, sys_init_module },     /* 154 = init_module (stub) */
    { 2, sys_delete_module },   /* 155 = delete_module (stub) */
    { 5, sys_mount2 },          /* 156 = mount (stub) */
    { 2, sys_umount2 },         /* 157 = umount2 (stub) */
    { 5, sys_name_to_handle_at }, /* 158 = name_to_handle_at (stub) */
    { 3, sys_open_by_handle_at }, /* 159 = open_by_handle_at (stub) */
    { 0, sys_enosys },     /* 160 = AUXV_H */
    { 0, sys_enosys },     /* 161 = MEMBARRIER_H */
    { 0, sys_enosys },     /* 162 = MMAN_H */
    { 0, sys_enosys },     /* 163 = SECCOMP */
    { 0, sys_enosys },     /* 164 = STAT_H */
    { 0, sys_enosys },     /* 165 = SYSINFO_H */
    { 0, sys_enosys },     /* 166 = TIME_H */
    { 0, sys_enosys },     /* 167 = USER_DISPATCH */
    { 0, sys_enosys },     /* 168 = _llseek */
    { 0, sys_enosys },     /* 169 = _newselect */
    { 0, sys_enosys },     /* 170 = accept */
    { 0, sys_enosys },     /* 171 = accept4 */
    { 0, sys_enosys },     /* 172 = acct */
    { 0, sys_enosys },     /* 173 = adjtimex */
    { 0, sys_enosys },     /* 174 = arch_prctl */
    { 0, sys_enosys },     /* 175 = cachectl */
    { 0, sys_enosys },     /* 176 = cacheflush */
    { 0, sys_enosys },     /* 177 = chown32 */
    { 0, sys_enosys },     /* 178 = clock_adjtime */
    { 0, sys_enosys },     /* 179 = clock_getres */
    { 0, sys_enosys },     /* 180 = clock_getres_time32 */
    { 0, sys_enosys },     /* 181 = clock_getres_time64 */
    { 0, sys_enosys },     /* 182 = clock_gettime */
    { 0, sys_enosys },     /* 183 = clock_gettime32 */
    { 0, sys_enosys },     /* 184 = clock_gettime64 */
    { 0, sys_enosys },     /* 185 = clock_nanosleep */
    { 0, sys_enosys },     /* 186 = clock_nanosleep_time32 */
    { 0, sys_enosys },     /* 187 = clock_nanosleep_time64 */
    { 0, sys_enosys },     /* 188 = clock_settime */
    { 0, sys_enosys },     /* 189 = clock_settime32 */
    { 0, sys_enosys },     /* 190 = clock_settime64 */
    { 0, fork },           /* 191 = clone (mapped to fork) */
    { 0, sys_enosys },     /* 192 = dup3 */
    { 0, sys_enosys },     /* 193 = epoll_create */
    { 0, sys_enosys },     /* 194 = epoll_wait */
    { 0, sys_enosys },     /* 195 = eventfd */
    { 0, sys_enosys },     /* 196 = execveat */
    { 0, sys_enosys },     /* 197 = faccessat */
    { 0, sys_enosys },     /* 198 = faccessat2 */
    { 0, sys_enosys },     /* 199 = fadvise64 */
    { 0, sys_enosys },     /* 200 = fadvise64_64 */
    { 0, sys_enosys },     /* 201 = fchdir */
    { 0, sys_enosys },     /* 202 = fchmodat */
    { 0, sys_enosys },     /* 203 = fchmodat2 */
    { 0, sys_enosys },     /* 204 = fchown32 */
    { 0, sys_enosys },     /* 205 = fchownat */
    { 0, sys_enosys },     /* 206 = fcntl64 */
    { 0, sys_enosys },     /* 207 = fdatasync */
    { 0, sys_enosys },     /* 208 = fgetxattr */
    { 0, sys_enosys },     /* 209 = flistxattr */
    { 0, sys_enosys },     /* 210 = fremovexattr */
    { 0, sys_enosys },     /* 211 = fsetxattr */
    { 0, sys_enosys },     /* 212 = fstat64 */
    { 0, sys_enosys },     /* 213 = fstatat */
    { 0, sys_enosys },     /* 214 = fstatat64 */
    { 0, sys_enosys },     /* 215 = fstatfs */
    { 0, sys_enosys },     /* 216 = fstatfs64 */
    { 0, sys_enosys },     /* 217 = fsync */
    { 0, sys_enosys },     /* 218 = ftruncate */
    { 0, sys_enosys },     /* 219 = ftruncate64 */
    { 0, sys_enosys },     /* 220 = futex */
    { 0, sys_enosys },     /* 221 = futimesat */
    { 0, sys_enosys },     /* 222 = get_robust_list */
    { 0, sys_enosys },     /* 223 = get_thread_area */
    { 0, sys_enosys },     /* 224 = getcpu */
    { 0, sys_enosys },     /* 225 = getcwd */
    { 0, sys_enosys },     /* 226 = getdents64 */
    { 0, sys_enosys },     /* 227 = getegid */
    { 0, sys_enosys },     /* 228 = getegid32 */
    { 0, sys_enosys },     /* 229 = geteuid */
    { 0, sys_enosys },     /* 230 = geteuid32 */
    { 0, sys_enosys },     /* 231 = getgid32 */
    { 0, sys_enosys },     /* 232 = getgroups */
    { 0, sys_enosys },     /* 233 = getgroups32 */
    { 0, sys_enosys },     /* 234 = getppid */
    { 0, sys_enosys },     /* 235 = getresgid32 */
    { 0, sys_enosys },     /* 236 = getresuid */
    { 0, sys_enosys },     /* 237 = getresuid32 */
    { 0, sys_enosys },     /* 238 = getrusage_time64 */
    { 0, sys_enosys },     /* 239 = gettid */
    { 0, sys_enosys },     /* 240 = gettimeofday_time32 */
    { 0, sys_enosys },     /* 241 = getuid32 */
    { 0, sys_enosys },     /* 242 = getxattr */
    { 0, sys_ok },         /* 243 = inotify_init (no-op) */
    { 0, sys_enosys },     /* 244 = ioperm */
    { 0, sys_enosys },     /* 245 = iopl */
    { 0, sys_enosys },     /* 246 = ipc */
    { 0, sys_enosys },     /* 247 = lchown32 */
    { 0, sys_enosys },     /* 248 = lgetxattr */
    { 0, sys_enosys },     /* 249 = linkat */
    { 0, sys_enosys },     /* 250 = listxattr */
    { 0, sys_enosys },     /* 251 = llistxattr */
    { 0, sys_enosys },     /* 252 = lremovexattr */
    { 0, sys_enosys },     /* 253 = lsetxattr */
    { 0, sys_enosys },     /* 254 = lstat64 */
    { 0, sys_enosys },     /* 255 = madvise */
    { 0, sys_enosys },     /* 256 = mincore */
    { 0, sys_enosys },     /* 257 = mkdirat */
    { 0, sys_enosys },     /* 258 = mknodat */
    { 0, sys_enosys },     /* 259 = mlock */
    { 0, sys_enosys },     /* 260 = mlockall */
    { 0, sys_enosys },     /* 261 = mmap2 */
    { 0, sys_enosys },     /* 262 = mq_getsetattr */
    { 0, sys_enosys },     /* 263 = mq_notify */
    { 0, sys_enosys },     /* 264 = mq_open */
    { 0, sys_enosys },     /* 265 = mq_timedreceive */
    { 0, sys_enosys },     /* 266 = mq_timedreceive_time64 */
    { 0, sys_enosys },     /* 267 = mq_timedsend */
    { 0, sys_enosys },     /* 268 = mq_timedsend_time64 */
    { 0, sys_enosys },     /* 269 = mq_unlink */
    { 0, sys_enosys },     /* 270 = mremap */
    { 0, sys_enosys },     /* 271 = msync */
    { 0, sys_enosys },     /* 272 = munlock */
    { 0, sys_enosys },     /* 273 = munlockall */
    { 0, sys_enosys },     /* 274 = newfstatat */
    { 0, sys_enosys },     /* 275 = personality */
    { 0, sys_enosys },     /* 276 = pipe2 */
    { 0, sys_enosys },     /* 277 = pivot_root */
    { 0, sys_enosys },     /* 278 = ppoll */
    { 0, sys_enosys },     /* 279 = ppoll_time64 */
    { 0, sys_enosys },     /* 280 = prctl */
    { 0, sys_enosys },     /* 281 = pread */
    { 0, sys_enosys },     /* 282 = pread64 */
    { 0, sys_enosys },     /* 283 = preadv */
    { 0, sys_enosys },     /* 284 = preadv2 */
    { 0, sys_enosys },     /* 285 = prlimit64 */
    { 0, sys_enosys },     /* 286 = process_vm_readv */
    { 0, sys_enosys },     /* 287 = process_vm_writev */
    { 0, sys_enosys },     /* 288 = pselect6 */
    { 0, sys_enosys },     /* 289 = pselect6_time64 */
    { 0, sys_enosys },     /* 290 = pwrite */
    { 0, sys_enosys },     /* 291 = pwrite64 */
    { 0, sys_enosys },     /* 292 = pwritev */
    { 0, sys_enosys },     /* 293 = pwritev2 */
    { 0, sys_enosys },     /* 294 = quotactl */
    { 0, sys_enosys },     /* 295 = readahead */
    { 0, sys_enosys },     /* 296 = readlink */
    { 0, sys_enosys },     /* 297 = readlinkat */
    { 0, sys_enosys },     /* 298 = readv */
    { 0, sys_enosys },     /* 299 = reboot */
    { 0, sys_enosys },     /* 300 = recvmmsg */
    { 0, sys_enosys },     /* 301 = recvmmsg_time64 */
    { 0, sys_enosys },     /* 302 = remap_file_pages */
    { 0, sys_enosys },     /* 303 = removexattr */
    { 0, sys_enosys },     /* 304 = renameat */
    { 0, sys_enosys },     /* 305 = renameat2 */
    { 0, sys_enosys },     /* 306 = riscv_flush_icache */
    { 0, sys_enosys },     /* 307 = rt_sigpending */
    { 0, sys_enosys },     /* 308 = rt_sigsuspend */
    { 0, sys_enosys },     /* 309 = rt_sigtimedwait */
    { 0, sys_enosys },     /* 310 = rt_sigtimedwait_time64 */
    { 0, sys_enosys },     /* 311 = sched_get_priority_max */
    { 0, sys_enosys },     /* 312 = sched_get_priority_min */
    { 0, sys_enosys },     /* 313 = sched_getparam */
    { 0, sys_enosys },     /* 314 = sched_getscheduler */
    { 0, sys_enosys },     /* 315 = sched_rr_get_interval */
    { 0, sys_enosys },     /* 316 = sched_rr_get_interval_time64 */
    { 0, sys_enosys },     /* 317 = sched_setaffinity */
    { 0, sys_enosys },     /* 318 = sched_setparam */
    { 0, sys_enosys },     /* 319 = sched_setscheduler */
    { 0, sys_enosys },     /* 320 = sched_yield */
    { 0, sys_enosys },     /* 321 = semtimedop */
    { 0, sys_enosys },     /* 322 = semtimedop_time64 */
    { 0, sys_enosys },     /* 323 = sendfile */
    { 0, sys_enosys },     /* 324 = sendfile64 */
    { 0, sys_enosys },     /* 325 = sendmmsg */
    { 0, sys_enosys },     /* 326 = set_robust_list */
    { 0, sys_enosys },     /* 327 = set_thread_area */
    { 0, sys_enosys },     /* 328 = setdomainname */
    { 0, sys_enosys },     /* 329 = setfsgid */
    { 0, sys_enosys },     /* 330 = setfsgid32 */
    { 0, sys_enosys },     /* 331 = setfsuid */
    { 0, sys_enosys },     /* 332 = setfsuid32 */
    { 0, sys_enosys },     /* 333 = setgid32 */
    { 0, sys_enosys },     /* 334 = setgroups */
    { 0, sys_enosys },     /* 335 = setgroups32 */
    { 0, sys_enosys },     /* 336 = sethostname */
    { 0, sys_enosys },     /* 337 = setns */
    { 0, sys_enosys },     /* 338 = setpriority */
    { 0, sys_enosys },     /* 339 = setregid */
    { 0, sys_enosys },     /* 340 = setregid32 */
    { 0, sys_enosys },     /* 341 = setresgid */
    { 0, sys_enosys },     /* 342 = setresgid32 */
    { 0, sys_enosys },     /* 343 = setresuid */
    { 0, sys_enosys },     /* 344 = setresuid32 */
    { 0, sys_enosys },     /* 345 = setreuid */
    { 0, sys_enosys },     /* 346 = setreuid32 */
    { 0, sys_enosys },     /* 347 = settimeofday */
    { 0, sys_enosys },     /* 348 = settimeofday_time32 */
    { 0, sys_enosys },     /* 349 = setuid32 */
    { 0, sys_enosys },     /* 350 = setxattr */
    { 0, sys_enosys },     /* 351 = sigaltstack */
    { 0, sys_enosys },     /* 352 = signalfd */
    { 0, sys_enosys },     /* 353 = signalfd4 */
    { 0, sys_enosys },     /* 354 = socketcall */
    { 0, sys_enosys },     /* 355 = splice */
    { 0, sys_enosys },     /* 356 = stat64 */
    { 0, sys_enosys },     /* 357 = statfs */
    { 0, sys_enosys },     /* 358 = statfs64 */
    { 0, sys_enosys },     /* 359 = statx */
    { 0, sys_enosys },     /* 360 = swapoff */
    { 0, sys_enosys },     /* 361 = swapon */
    { 0, sys_enosys },     /* 362 = symlink */
    { 0, sys_enosys },     /* 363 = symlinkat */
    { 0, sys_enosys },     /* 364 = sync_file_range */
    { 0, sys_enosys },     /* 365 = sync_file_range2 */
    { 0, sys_enosys },     /* 366 = syncfs */
    { 0, sys_enosys },     /* 367 = sysinfo */
    { 0, sys_enosys },     /* 368 = tee */
    { 0, sys_enosys },     /* 369 = timer_create */
    { 0, sys_enosys },     /* 370 = timer_delete */
    { 0, sys_enosys },     /* 371 = timer_getoverrun */
    { 0, sys_enosys },     /* 372 = timer_gettime */
    { 0, sys_enosys },     /* 373 = timer_gettime32 */
    { 0, sys_enosys },     /* 374 = timer_gettime64 */
    { 0, sys_enosys },     /* 375 = timer_settime */
    { 0, sys_enosys },     /* 376 = timer_settime32 */
    { 0, sys_enosys },     /* 377 = timer_settime64 */
    { 0, sys_enosys },     /* 378 = timerfd_create */
    { 0, sys_enosys },     /* 379 = timerfd_gettime */
    { 0, sys_enosys },     /* 380 = timerfd_gettime32 */
    { 0, sys_enosys },     /* 381 = timerfd_gettime64 */
    { 0, sys_enosys },     /* 382 = timerfd_settime */
    { 0, sys_enosys },     /* 383 = timerfd_settime32 */
    { 0, sys_enosys },     /* 384 = timerfd_settime64 */
    { 0, sys_enosys },     /* 385 = truncate */
    { 0, sys_enosys },     /* 386 = truncate64 */
    { 0, sys_enosys },     /* 387 = ugetrlimit */
    { 0, sys_enosys },     /* 388 = umask */
    { 0, sys_enosys },     /* 389 = uname */
    { 0, sys_enosys },     /* 390 = unlinkat */
    { 0, sys_enosys },     /* 391 = unshare */
    { 0, sys_enosys },     /* 392 = utimensat */
    { 0, sys_enosys },     /* 393 = utimensat_time64 */
    { 0, sys_enosys },     /* 394 = vhangup */
    { 0, sys_enosys },     /* 395 = vmsplice */
    { 0, sys_enosys },     /* 396 = wait4 */
    { 0, sys_enosys },     /* 397 = wait4_time64 */
    { 0, sys_enosys },     /* 398 = writev */
    { 0, sys_enosys },     /* 399 = socketcall */
    { 0, sys_enosys },     /* 400 = socket */
    { 0, sys_enosys },     /* 401 = bind */
    { 0, sys_enosys },     /* 402 = connect */
    { 0, sys_enosys },     /* 403 = listen */
    { 0, sys_enosys },     /* 404 = accept */
    { 0, sys_enosys },     /* 405 = getsockname */
    { 0, sys_enosys },     /* 406 = getpeername */
    { 0, sys_enosys },     /* 407 = socketpair */
    { 0, sys_enosys },     /* 408 = send */
    { 0, sys_enosys },     /* 409 = recv */
    { 0, sys_enosys },     /* 410 = sendto */
    { 0, sys_enosys },     /* 411 = recvfrom */
    { 0, sys_enosys },     /* 412 = shutdown */
    { 0, sys_enosys },     /* 413 = setsockopt */
    { 0, sys_enosys },     /* 414 = getsockopt */
    { 0, sys_enosys },     /* 415 = sendmsg */
    { 0, sys_enosys },     /* 416 = recvmsg */
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
