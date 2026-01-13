/* filsys.h - Unix V6 x86 Port Filesystem Superblock
 * Ported from original V6 filsys.h for PDP-11
 * Superblock structure for mounted filesystems
 */

#ifndef _FILSYS_H_
#define _FILSYS_H_

#include "param.h"
#include "types.h"

/*
 * Definition of the Unix file system superblock.
 * The root superblock is read in at boot time and
 * copied into the mount table. Other filesystems
 * get their superblocks read when mounted.
 */
struct filsys {
    uint16_t    s_isize;        /* Size in blocks of I-list */
    daddr_t     s_fsize;        /* Size in blocks of entire volume */
    int16_t     s_nfree;        /* Number of addresses in s_free */
    daddr_t     s_free[100];    /* Free block list */
    int16_t     s_ninode;       /* Number of inodes in s_inode */
    ino_t       s_inode[100];   /* Free inode list */
    int8_t      s_flock;        /* Lock during free list manipulation */
    int8_t      s_ilock;        /* Lock during I-list manipulation */
    int8_t      s_fmod;         /* Superblock modified flag */
    int8_t      s_ronly;        /* Mounted read-only flag */
    time_t      s_time[2];      /* Last super block update (64-bit time) */
    /* Padding to fill 512-byte block */
};

/*
 * Structure of an on-disk inode (32 bytes in V6)
 */
struct dinode {
    mode_t      di_mode;        /* File type and permissions */
    int8_t      di_nlink;       /* Number of links */
    uid_t       di_uid;         /* Owner user ID */
    gid_t       di_gid;         /* Owner group ID */
    uint8_t     di_size0;       /* High byte of size */
    uint16_t    di_size1;       /* Low 16 bits of size */
    uint16_t    di_addr[8];     /* Block addresses (encoded) */
    time_t      di_atime;       /* Access time */
    time_t      di_mtime;       /* Modification time */
};

/*
 * Directory entry structure (16 bytes in V6)
 */
struct direct {
    ino_t       d_ino;          /* Inode number */
    char        d_name[DIRSIZ]; /* Filename */
};

/*
 * Filesystem function prototypes
 */
struct filsys *getfs(dev_t dev);
struct buf *alloc(dev_t dev);
void bfree(dev_t dev, daddr_t bno);
int badblock(struct filsys *fp, daddr_t bno, dev_t dev);

#endif /* _FILSYS_H_ */
