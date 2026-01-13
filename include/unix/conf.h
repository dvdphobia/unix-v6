/* conf.h - Device Configuration
 * Unix V6 x86 Port
 */

#ifndef _UNIX_CONF_H
#define _UNIX_CONF_H

#include <unix/types.h>

struct buf;

/* Block device switch */
struct bdevsw {
    int     (*d_open)(dev_t dev, int flag);
    int     (*d_close)(dev_t dev, int flag);
    int     (*d_strategy)(struct buf *bp);
    struct devtab *d_tab;
};

/* Character device switch */
struct cdevsw {
    int     (*d_open)(dev_t dev, int flag);
    int     (*d_close)(dev_t dev, int flag);
    int     (*d_read)(dev_t dev);
    int     (*d_write)(dev_t dev);
    int     (*d_sgtty)(dev_t dev, int flag);
};

/* External device tables */
extern struct bdevsw bdevsw[];
extern struct cdevsw cdevsw[];
extern int nblkdev;
extern int nchrdev;

#endif /* _UNIX_CONF_H */
