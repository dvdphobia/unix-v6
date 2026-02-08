#ifndef _SYS_FB_H
#define _SYS_FB_H

#include <stdint.h>

struct fb_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    void *addr;
};

int getfbinfo(struct fb_info *info);

#endif
