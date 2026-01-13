/* filsys.h - Filesystem Superblock
 * Unix V6 x86 Port
 */

#ifndef _UNIX_FILSYS_H
#define _UNIX_FILSYS_H

#include <unix/types.h>
#include <unix/param.h>

/* Superblock structure - on disk and in core */
struct filsys {
    uint16_t    s_isize;        /* Size in blocks of I list */
    daddr_t     s_fsize;        /* Size in blocks of entire volume */
    int16_t     s_nfree;        /* Number of in-core free blocks */
    daddr_t     s_free[NICFREE];/* In-core free blocks */
    int16_t     s_ninode;       /* Number of in-core I nodes */
    ino_t       s_inode[NICINOD];/* In-core free I nodes */
    int8_t      s_flock;        /* Lock during free list manipulation */
    int8_t      s_ilock;        /* Lock during I list manipulation */
    int8_t      s_fmod;         /* Superblock modified flag */
    int8_t      s_ronly;        /* Mounted read-only flag */
    time_t      s_time[2];      /* Current date of last update */
    int16_t     pad[50];        /* Padding to 512 bytes */
};

/* Mount table entry */
struct mount {
    dev_t       m_dev;          /* Device number */
    struct buf  *m_bufp;        /* Pointer to superblock */
    struct inode *m_inodp;      /* Pointer to mounted-on inode */
};

extern struct mount mount[];

#endif /* _UNIX_FILSYS_H */
