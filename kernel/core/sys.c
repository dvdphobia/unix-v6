/* sys.c - System Call Interface
 * Unix V6 x86 Port
 * Ported from original V6 ken/sys1.c, sys2.c, sys3.c, sys4.c
 */

#include <unix/types.h>
#include <unix/param.h>
#include <unix/proc.h>
#include <unix/user.h>
#include <unix/inode.h>
#include <unix/file.h>
#include <unix/buf.h>

extern void printf(const char *fmt, ...);
extern void panic(const char *msg);
extern struct proc proc[];
extern struct proc *curproc;
extern struct user u;
extern struct file file[];
extern struct inode inode[];
extern time_t time[];

/*
 * System call entry table
 */
struct sysent {
    int     narg;           /* Number of arguments */
    int     (*call)(void);  /* Handler function */
};

/* Forward declarations */
static int sys_nosys(void);
static int sys_exit(void);
static int sys_fork(void);
static int sys_read(void);
static int sys_write(void);
static int sys_open(void);
static int sys_close(void);
static int sys_wait(void);
static int sys_creat(void);
static int sys_link(void);
static int sys_unlink(void);
static int sys_chdir(void);
static int sys_time(void);
static int sys_chmod(void);
static int sys_chown(void);
static int sys_stat(void);
static int sys_seek(void);
static int sys_getpid(void);
static int sys_getuid(void);
static int sys_getgid(void);
static int sys_dup(void);
static int sys_pipe(void);
static int sys_sync(void);
static int sys_kill(void);

/*
 * System call table - 64 entries like original V6
 */
struct sysent sysent[] = {
    { 0, sys_nosys },       /*  0 = indir (indirect syscall) */
    { 0, sys_exit },        /*  1 = exit */
    { 0, sys_fork },        /*  2 = fork */
    { 2, sys_read },        /*  3 = read */
    { 2, sys_write },       /*  4 = write */
    { 2, sys_open },        /*  5 = open */
    { 0, sys_close },       /*  6 = close */
    { 0, sys_wait },        /*  7 = wait */
    { 2, sys_creat },       /*  8 = creat */
    { 2, sys_link },        /*  9 = link */
    { 1, sys_unlink },      /* 10 = unlink */
    { 2, sys_nosys },       /* 11 = exec */
    { 1, sys_chdir },       /* 12 = chdir */
    { 0, sys_time },        /* 13 = time */
    { 3, sys_nosys },       /* 14 = mknod */
    { 2, sys_chmod },       /* 15 = chmod */
    { 2, sys_chown },       /* 16 = chown */
    { 1, sys_nosys },       /* 17 = break (sbrk) */
    { 2, sys_stat },        /* 18 = stat */
    { 2, sys_seek },        /* 19 = seek (lseek) */
    { 0, sys_getpid },      /* 20 = getpid */
    { 3, sys_nosys },       /* 21 = mount */
    { 1, sys_nosys },       /* 22 = umount */
    { 0, sys_nosys },       /* 23 = setuid */
    { 0, sys_getuid },      /* 24 = getuid */
    { 0, sys_nosys },       /* 25 = stime */
    { 3, sys_nosys },       /* 26 = ptrace */
    { 0, sys_nosys },       /* 27 = x */
    { 1, sys_nosys },       /* 28 = fstat */
    { 0, sys_nosys },       /* 29 = x */
    { 1, sys_nosys },       /* 30 = smdate */
    { 1, sys_nosys },       /* 31 = stty */
    { 1, sys_nosys },       /* 32 = gtty */
    { 0, sys_nosys },       /* 33 = x */
    { 0, sys_nosys },       /* 34 = nice */
    { 0, sys_nosys },       /* 35 = sleep */
    { 0, sys_sync },        /* 36 = sync */
    { 1, sys_kill },        /* 37 = kill */
    { 0, sys_nosys },       /* 38 = switch */
    { 0, sys_nosys },       /* 39 = x */
    { 0, sys_nosys },       /* 40 = x */
    { 0, sys_dup },         /* 41 = dup */
    { 0, sys_pipe },        /* 42 = pipe */
    { 1, sys_nosys },       /* 43 = times */
    { 4, sys_nosys },       /* 44 = prof */
    { 0, sys_nosys },       /* 45 = x */
    { 0, sys_nosys },       /* 46 = setgid */
    { 0, sys_getgid },      /* 47 = getgid */
    { 2, sys_nosys },       /* 48 = sig (signal) */
};

#define NSYSENT (sizeof(sysent) / sizeof(sysent[0]))

/*
 * nosys - Unimplemented system call
 */
static int sys_nosys(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * exit - Terminate process
 */
static int sys_exit(void) {
    struct proc *p = curproc;
    int i;
    
    /* Close all open files */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] != NULL) {
            closef(u.u_ofile[i]);
            u.u_ofile[i] = NULL;
        }
    }
    
    /* Release current directory */
    if (u.u_cdir) {
        iput(u.u_cdir);
        u.u_cdir = NULL;
    }
    
    /* Mark process as zombie */
    p->p_stat = SZOMB;
    
    /* Wake up parent */
    wakeup(p->p_ppid);
    
    /* Orphan children to init (process 1) */
    for (i = 0; i < NPROC; i++) {
        if (proc[i].p_ppid == p->p_pid) {
            proc[i].p_ppid = 1;
        }
    }
    
    swtch();
    return 0;
}

/*
 * fork - Create new process
 */
static int sys_fork(void) {
    int pid = newproc();
    if (pid < 0) {
        return -1;
    }
    u.u_rv1 = pid;
    return 0;
}

/*
 * read - Read from file
 */
static int sys_read(void) {
    struct file *fp;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) return -1;
    
    if ((fp->f_flag & FREAD) == 0) {
        u.u_error = EBADF;
        return -1;
    }
    
    u.u_base = (char *)u.u_arg[1];
    u.u_count = u.u_arg[2];
    u.u_offset[0] = 0;
    u.u_offset[1] = fp->f_offset;
    u.u_segflg = 0;
    
    readi(fp->f_inode);
    
    fp->f_offset += u.u_arg[2] - u.u_count;
    u.u_rv1 = u.u_arg[2] - u.u_count;
    
    return 0;
}

/*
 * write - Write to file
 */
static int sys_write(void) {
    struct file *fp;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) return -1;
    
    if ((fp->f_flag & FWRITE) == 0) {
        u.u_error = EBADF;
        return -1;
    }
    
    u.u_base = (char *)u.u_arg[1];
    u.u_count = u.u_arg[2];
    u.u_offset[0] = 0;
    u.u_offset[1] = fp->f_offset;
    u.u_segflg = 0;
    
    writei(fp->f_inode);
    
    fp->f_offset += u.u_arg[2] - u.u_count;
    u.u_rv1 = u.u_arg[2] - u.u_count;
    
    return 0;
}

/*
 * open - Open file
 */
static int sys_open(void) {
    struct inode *ip;
    struct file *fp;
    int mode = u.u_arg[1];
    int i;
    
    ip = namei(NULL, 0);
    if (ip == NULL) {
        u.u_error = ENOENT;
        return -1;
    }
    
    /* Find free file table entry */
    fp = NULL;
    for (i = 0; i < NFILE; i++) {
        if (file[i].f_count == 0) {
            fp = &file[i];
            break;
        }
    }
    if (fp == NULL) {
        iput(ip);
        u.u_error = ENFILE;
        return -1;
    }
    
    /* Find free fd in process */
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_ofile[i] = fp;
            break;
        }
    }
    if (i >= NOFILE) {
        iput(ip);
        u.u_error = EMFILE;
        return -1;
    }
    
    fp->f_flag = (mode & 3) ? FWRITE : FREAD;
    if ((mode & 3) == 2) fp->f_flag = FREAD | FWRITE;
    fp->f_count = 1;
    fp->f_inode = ip;
    fp->f_offset = 0;
    
    u.u_rv1 = i;
    return 0;
}

/*
 * close - Close file descriptor
 */
static int sys_close(void) {
    int fd = u.u_arg[0];
    struct file *fp;
    
    if (fd < 0 || fd >= NOFILE) {
        u.u_error = EBADF;
        return -1;
    }
    
    fp = u.u_ofile[fd];
    if (fp == NULL) {
        u.u_error = EBADF;
        return -1;
    }
    
    u.u_ofile[fd] = NULL;
    closef(fp);
    
    return 0;
}

/*
 * wait - Wait for child process
 */
static int sys_wait(void) {
    struct proc *p;
    int i, found;
    
    for (;;) {
        found = 0;
        
        for (i = 0; i < NPROC; i++) {
            p = &proc[i];
            if (p->p_ppid == curproc->p_pid) {
                found = 1;
                if (p->p_stat == SZOMB) {
                    u.u_rv1 = p->p_pid;
                    u.u_rv2 = 0;  /* Exit status */
                    p->p_stat = SNULL;
                    return 0;
                }
            }
        }
        
        if (!found) {
            u.u_error = ECHILD;
            return -1;
        }
        
        sleep(&curproc->p_pid, PWAIT);
    }
}

/*
 * creat - Create file
 */
static int sys_creat(void) {
    /* Stub - would create file with namei/maknode */
    u.u_error = ENOSYS;
    return -1;
}

/*
 * link - Create link to file
 */
static int sys_link(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * unlink - Remove file
 */
static int sys_unlink(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * chdir - Change directory
 */
static int sys_chdir(void) {
    struct inode *ip;
    
    ip = namei(NULL, 0);
    if (ip == NULL) {
        u.u_error = ENOENT;
        return -1;
    }
    
    if ((ip->i_mode & IFMT) != IFDIR) {
        iput(ip);
        u.u_error = ENOTDIR;
        return -1;
    }
    
    if (u.u_cdir) iput(u.u_cdir);
    u.u_cdir = ip;
    
    return 0;
}

/*
 * time - Get system time
 */
static int sys_time(void) {
    u.u_rv1 = time[0];
    u.u_rv2 = time[1];
    return 0;
}

/*
 * chmod - Change file mode
 */
static int sys_chmod(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * chown - Change file owner
 */
static int sys_chown(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * stat - Get file status
 */
static int sys_stat(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * seek - Seek in file
 */
static int sys_seek(void) {
    struct file *fp;
    
    fp = getf(u.u_arg[0]);
    if (fp == NULL) return -1;
    
    switch (u.u_arg[2]) {
    case 0:  /* SEEK_SET */
        fp->f_offset = u.u_arg[1];
        break;
    case 1:  /* SEEK_CUR */
        fp->f_offset += u.u_arg[1];
        break;
    case 2:  /* SEEK_END */
        fp->f_offset = fp->f_inode->i_size + u.u_arg[1];
        break;
    }
    
    u.u_rv1 = fp->f_offset;
    return 0;
}

/*
 * getpid - Get process ID
 */
static int sys_getpid(void) {
    u.u_rv1 = curproc->p_pid;
    return 0;
}

/*
 * getuid - Get user ID
 */
static int sys_getuid(void) {
    u.u_rv1 = u.u_ruid;
    u.u_rv2 = u.u_uid;
    return 0;
}

/*
 * getgid - Get group ID
 */
static int sys_getgid(void) {
    u.u_rv1 = u.u_rgid;
    u.u_rv2 = u.u_gid;
    return 0;
}

/*
 * dup - Duplicate file descriptor
 */
static int sys_dup(void) {
    struct file *fp;
    int fd = u.u_arg[0];
    int i;
    
    fp = getf(fd);
    if (fp == NULL) return -1;
    
    for (i = 0; i < NOFILE; i++) {
        if (u.u_ofile[i] == NULL) {
            u.u_ofile[i] = fp;
            fp->f_count++;
            u.u_rv1 = i;
            return 0;
        }
    }
    
    u.u_error = EMFILE;
    return -1;
}

/*
 * pipe - Create pipe
 */
static int sys_pipe(void) {
    u.u_error = ENOSYS;
    return -1;
}

/*
 * sync - Sync filesystems
 */
static int sys_sync(void) {
    /* Would flush all buffers here */
    return 0;
}

/*
 * kill - Send signal
 */
static int sys_kill(void) {
    syskill();
    return u.u_error ? -1 : 0;
}

/*
 * syscall - System call dispatcher
 */
void syscall(int num) {
    struct sysent *sp;
    
    if (num < 0 || num >= NSYSENT) {
        u.u_error = ENOSYS;
        return;
    }
    
    sp = &sysent[num];
    u.u_error = 0;
    
    (*sp->call)();
}
