#include <stdio.h>
#include <stddef.h>

/* Include the kernel header files */
#define NSIG 20
#define NOFILE 15
#define DIRSIZ 14
#define USIZE 64
#define USIZE_BYTES (USIZE * 64)

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef int int8_t;
typedef unsigned int mode_t;
typedef int off_t;
typedef int ino_t;
typedef char *caddr_t;

struct inode { int dummy; };
struct file { int dummy; };
struct proc { int dummy; };

struct user {
    uint32_t    u_rsav[6];
    uint32_t    u_fsav[8];
    int8_t      u_segflg;
    int8_t      u_error;
    uint8_t     u_uid;
    uint8_t     u_gid;
    uint8_t     u_ruid;
    uint8_t     u_rgid;
    int         u_rval;
    mode_t      u_cmask;
    struct proc *u_procp;
    caddr_t     u_base;
    uint32_t    u_count;
    off_t       u_offset[2];
    struct inode *u_cdir;
    char        u_dbuf[DIRSIZ];
    caddr_t     u_dirp;
    struct {
        ino_t   u_ino;
        char    u_name[DIRSIZ];
    } u_dent;
    struct inode *u_pdir;
    uint32_t    u_uisa[16];
    uint32_t    u_uisd[16];
    struct file *u_ofile[NOFILE];
    uint32_t    u_arg[5];
    uint32_t    *u_ar0;
    uint32_t    u_tsize;
    uint32_t    u_dsize;
    uint32_t    u_ssize;
    int32_t     u_sep;
    uint32_t    u_qsav[6];
    uint32_t    u_ssav[6];
    uint32_t    u_signal[NSIG];
    uint32_t    u_sigmask[2];
    uint32_t    u_pendingsig[2];
    uint32_t    u_utime;
    uint32_t    u_stime;
    uint32_t    u_cutime[2];
    uint32_t    u_cstime[2];
    uint32_t    u_prof[4];
    int8_t      u_intflg;
    uint8_t     u_stack[3549];
};

int main() {
    printf("sizeof(struct user) = %ld\n", sizeof(struct user));
    printf("USIZE_BYTES = %d\n", USIZE_BYTES);
    printf("Difference = %ld\n", (long)sizeof(struct user) - USIZE_BYTES);
    printf("\nField offsets:\n");
    printf("  u_signal: %ld (size %ld)\n", offsetof(struct user, u_signal), sizeof(((struct user*)0)->u_signal));
    printf("  u_sigmask: %ld (size %ld)\n", offsetof(struct user, u_sigmask), sizeof(((struct user*)0)->u_sigmask));
    printf("  u_pendingsig: %ld (size %ld)\n", offsetof(struct user, u_pendingsig), sizeof(((struct user*)0)->u_pendingsig));
    printf("  u_utime: %ld (size %ld)\n", offsetof(struct user, u_utime), sizeof(((struct user*)0)->u_utime));
    printf("  u_stack: %ld (size %ld)\n", offsetof(struct user, u_stack), sizeof(((struct user*)0)->u_stack));
    
    return 0;
}
