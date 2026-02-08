
#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/systm.h"
#include "include/multiboot.h"

struct fb_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    void *addr;
};

/* Defined in trap.c or assembly (m40.s equivalent) */
extern int copyout(const void *src, void *dst, int count);

/*
 * getfbinfo - Get Framebuffer Information
 * Syscall 39
 * args: struct fb_info *info
 */
int getfbinfo(void) {
    struct fb_info info;
    struct fb_info *u_info = (struct fb_info*)u.u_arg[0];
    
    if (!fb_addr) {
        u.u_error = ENODEV;
        return -1;
    }
    
    info.width = fb_width;
    info.height = fb_height;
    info.pitch = fb_pitch;
    info.bpp = fb_bpp;
    info.addr = fb_addr;
    
    if (copyout(&info, u_info, sizeof(info)) < 0) {
        u.u_error = EFAULT;
        return -1;
    }
    return 0;
}
