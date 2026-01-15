#include "include/types.h"
#include "include/conf.h"

static int drv_open(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

static int drv_close(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

static int drv_strategy(struct buf *bp) {
    (void)bp;
    return 0;
}

struct bdevsw drv_bdevsw = {
    .d_open = drv_open,
    .d_close = drv_close,
    .d_strategy = drv_strategy,
    .d_tab = 0,
};
