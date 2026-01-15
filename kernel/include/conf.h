/* conf.h - Unix V6 x86 Port Device Configuration
 * Ported from original V6 conf.h for PDP-11
 * Device switch tables and configuration
 */

#ifndef _CONF_H_
#define _CONF_H_

#include "param.h"
#include "types.h"

/*
 * Block device switch table entry
 * One entry per block device type (disk, etc.)
 */
struct buf;
struct bdevsw {
    int     (*d_open)(dev_t dev, int flag);     /* Open device */
    int     (*d_close)(dev_t dev, int flag);    /* Close device */
    int     (*d_strategy)(struct buf *bp);      /* Start I/O */
    struct devtab *d_tab;                        /* Device table */
};

/* Number of block device types */
#define NBLKDEV     4

/* Block device switch table */
extern struct bdevsw bdevsw[NBLKDEV];

/*
 * Character device switch table entry
 * One entry per character device type (tty, etc.)
 */
struct cdevsw {
    int     (*d_open)(dev_t dev, int flag);     /* Open device */
    int     (*d_close)(dev_t dev, int flag);    /* Close device */
    int     (*d_read)(dev_t dev);               /* Read */
    int     (*d_write)(dev_t dev);              /* Write */
    int     (*d_sgtty)(dev_t dev, void *v);     /* Get/set terminal modes */
};

/* Number of character device types */
#define NCHRDEV     8

/* Character device switch table */
extern struct cdevsw cdevsw[NCHRDEV];

/*
 * Device number manipulation
 * Uses same major/minor scheme as V6
 */
#define NODEV       ((dev_t)-1)

/*
 * Root and swap configuration
 * Set at boot time based on hardware
 */
extern dev_t rootdev;           /* Root filesystem device */
extern dev_t swapdev;           /* Swap device */
extern dev_t pipedev;           /* Pipe device (usually same as root) */
extern daddr_t swplo;           /* Starting block of swap */
extern int nswap;               /* Number of swap blocks */

/*
 * Null device functions (for unused slots)
 */
int nulldev(void);
int nodev(void);

#endif /* _CONF_H_ */
