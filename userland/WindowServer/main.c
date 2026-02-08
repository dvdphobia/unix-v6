#include <stdio.h>
#include <stdlib.h>
#include <sys/fb.h>

typedef uint32_t color_t;

void draw_rect(struct fb_info *fb, int x, int y, int w, int h, color_t color) {
    if (!fb->addr) return;
    
    for (int j = 0; j < h; j++) {
        if (y+j < 0 || y+j >= (int)fb->height) continue;
        
        uint32_t *row = (uint32_t*)((uint8_t*)fb->addr + (y+j)*fb->pitch);
        for (int i = 0; i < w; i++) {
            if (x+i < 0 || x+i >= (int)fb->width) continue;
            
            row[x+i] = color;
        }
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("Starting WindowServer...\n");
    
    struct fb_info fb;
    if (getfbinfo(&fb) < 0) {
        printf("WindowServer: Failed to get framebuffer info.\n");
        return 1;
    }
    
    printf("Display: %dx%dx%d at %p\n", fb.width, fb.height, fb.bpp, fb.addr);
    
    /* Draw Wallpaper (Teal) */
    draw_rect(&fb, 0, 0, fb.width, fb.height, 0x00008080);
    
    /* Draw Window Frame */
    int wx = 150, wy = 100, ww = 500, wh = 350;
    
    /* Shadow */
    draw_rect(&fb, wx+10, wy+10, ww, wh, 0x00000000);
    
    /* Background */
    draw_rect(&fb, wx, wy, ww, wh, 0x00C0C0C0);
    
    /* Title Bar */
    draw_rect(&fb, wx+2, wy+2, ww-4, 25, 0x00000080);
    
    printf("WindowServer initialized.\n");
    
    while(1) {
        /* TODO: Read mouse/keyboard */
        /* For now, just spin */
    }
    
    return 0;
}
