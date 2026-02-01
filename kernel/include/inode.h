/* inode.h - Unix V6 x86 Port Inode Structure
 * Ported from original V6 inode.h for PDP-11
 * In-core inode representation
 */

#ifndef _INODE_H_
#define _INODE_H_

#include "param.h"
#include "types.h"

/*
 * The inode is the focus of all file activity in Unix.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file,
 * and the root.
 *
 * An inode is 'named' by its dev/inumber pair (iget/iget.c).
 * Data from mode onwards is read from the permanent inode on volume.
 */
struct inode {
    int8_t      i_flag;         /* Inode flags */
    int8_t      i_count;        /* Reference count */
    dev_t       i_dev;          /* Device where inode resides */
    ino_t       i_number;       /* Inode number (1-to-1 with disk address) */
    mode_t      i_mode;         /* File type and permissions */
    int8_t      i_nlink;        /* Number of directory entries */
    uid_t       i_uid;          /* Owner user ID */
    gid_t       i_gid;          /* Owner group ID */
    uint8_t     i_size0;        /* Most significant byte of size */
    uint32_t    i_size1;        /* Least significant bytes of size */
    daddr_t     i_addr[8];      /* Disk block addresses */
    blkno_t     i_lastr;        /* Last logical block read (for read-ahead) */
    time_t      i_atime;        /* Last access time */
    time_t      i_mtime;        /* Last modification time */
    time_t      i_ctime;        /* Last status change time */
};

/* Inode flags */
#define ILOCK       01          /* Inode is locked */
#define IUPD        02          /* Inode has been modified */
#define IACC        04          /* Inode access time to be updated */
#define IMOUNT      010         /* Inode is mounted on */
#define IWANT       020         /* Some process waiting on lock */
#define ITEXT       040         /* Inode is pure text prototype */
#define ICHG        0100        /* Inode metadata changed */

/* File type and mode bits */
#define IALLOC      0100000     /* File is used (allocated) */
#define IFMT        060000      /* Type of file mask */
#define IFDIR       040000      /* Directory */
#define IFCHR       020000      /* Character special */
#define IFBLK       060000      /* Block special (0 = regular) */
#define IFREG       000000      /* Regular file */
#define IFIFO       010000      /* FIFO/Named pipe */
#define ILARG       010000      /* Large addressing algorithm */
#define ISUID       04000       /* Set user ID on execution */
#define ISGID       02000       /* Set group ID on execution */
#define ISVTX       01000       /* Save swapped text even after use */
#define IREAD       0400        /* Read permission */
#define IWRITE      0200        /* Write permission */
#define IEXEC       0100        /* Execute permission */

/* Global inode table */
extern struct inode inode[NINODE];

/* Root directory inode pointer */
extern struct inode *rootdir;

/*
 * Inode function prototypes
 */
struct inode *iget(dev_t dev, ino_t ino);
void iput(struct inode *ip);
void iupdat(struct inode *ip, time_t *tm);
void itrunc(struct inode *ip);
struct inode *ialloc(dev_t dev);
void ifree(dev_t dev, ino_t ino);
struct inode *namei(int (*func)(void), int flag);
void prele(struct inode *ip);
int access(struct inode *ip, int mode);
void iinit(void);

#endif /* _INODE_H_ */
