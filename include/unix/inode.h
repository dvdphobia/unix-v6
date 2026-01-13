/* inode.h - In-Core Inode Structure
 * Unix V6 x86 Port
 */

#ifndef _UNIX_INODE_H
#define _UNIX_INODE_H

#include <unix/types.h>
#include <unix/param.h>

/*
 * In-core inode structure
 */
struct inode {
    int8_t      i_flag;         /* Flags */
    int8_t      i_count;        /* Reference count */
    dev_t       i_dev;          /* Device where inode resides */
    ino_t       i_number;       /* Inode number */
    mode_t      i_mode;         /* Mode and type of file */
    int8_t      i_nlink;        /* Number of links */
    uid_t       i_uid;          /* Owner */
    gid_t       i_gid;          /* Group */
    off_t       i_size;         /* Size of file */
    daddr_t     i_addr[8];      /* Block addresses */
    time_t      i_atime;        /* Access time */
    time_t      i_mtime;        /* Modification time */
};

/* Inode flags */
#define ILOCK   0x01            /* Inode is locked */
#define IUPD    0x02            /* Inode has been modified */
#define IACC    0x04            /* Inode access time to be updated */
#define IMOUNT  0x08            /* Inode is mounted on */
#define IWANT   0x10            /* Some process waiting on lock */
#define ITEXT   0x20            /* Inode is pure text prototype */

/* File modes */
#define IFMT    0170000         /* Type of file */
#define IFDIR   0040000         /* Directory */
#define IFCHR   0020000         /* Character special */
#define IFBLK   0060000         /* Block special */
#define IFREG   0100000         /* Regular */
#define IFLNK   0120000         /* Symbolic link */
#define IFSOCK  0140000         /* Socket */

#define ISUID   0004000         /* Set user id on execution */
#define ISGID   0002000         /* Set group id on execution */
#define ISVTX   0001000         /* Save swapped text after use */
#define IREAD   0000400         /* Read permission */
#define IWRITE  0000200         /* Write permission */
#define IEXEC   0000100         /* Execute permission */

/* Function prototypes */
struct inode *iget(dev_t dev, ino_t ino);
void iput(struct inode *ip);
void iupdat(struct inode *ip, time_t *ta, time_t *tm);
void itrunc(struct inode *ip);
struct inode *maknode(int mode);
struct inode *namei(int (*func)(void), int flag);
void wdir(struct inode *ip);
struct inode *ialloc(dev_t dev);
void ifree(dev_t dev, ino_t ino);
daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg);

extern struct inode inode[];
extern struct inode *rootdir;

#endif /* _UNIX_INODE_H */
