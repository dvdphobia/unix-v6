#include "include/types.h"
#include "include/multiboot.h"
#include "include/param.h"

/* Global framebuffer info */
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_bpp = 0;
uint32_t fb_pitch = 0;
void *fb_addr = 0;

static uint32_t cur_x = 0;
static uint32_t cur_y = 0;

void fb_init(multiboot_info_t *mbi) {
    fb_addr = (void*)(uint32_t)mbi->framebuffer_addr;
    fb_width = mbi->framebuffer_width;
    fb_height = mbi->framebuffer_height;
    fb_pitch = mbi->framebuffer_pitch;
    fb_bpp = mbi->framebuffer_bpp;
    
    cur_x = 0;
    cur_y = 0;
    fb_clear(); /* Clear screen on init */
}

void fb_put_pixel(int x, int y, uint32_t color) {
    if (!fb_addr) return;
    if (x < 0 || x >= (int)fb_width || y < 0 || y >= (int)fb_height) return;
    
    /* Assume 32-bpp for now */
    uint32_t *pixel = (uint32_t*)((uint8_t*)fb_addr + y * fb_pitch + x * 4);
    *pixel = color;
}

void fb_clear(void) {
    if (!fb_addr) return;
    /* Slow clear */
    for (uint32_t y = 0; y < fb_height; y++) {
        uint32_t *row = (uint32_t*)((uint8_t*)fb_addr + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; x++) {
            row[x] = 0; /* Black */
        }
    }
    cur_x = 0;
    cur_y = 0;
}

#include "font.h"

void fb_putc(char c) {
    if (!fb_addr) return;
    
    if (c == '\n') {
        cur_x = 0;
        cur_y += 10;
        /* Scroll check omitted for brevity */
        return;
    }
    if (c == '\r') {
        cur_x = 0;
        return;
    }
    
    if (cur_x + 8 >= fb_width) {
        cur_x = 0;
        cur_y += 10;
    }
    
    /* Draw char using 8x8 font */
    int idx = (unsigned char)c;
    if (idx > 127) idx = 127; /* Cap at 127 */
    
    for (int y = 0; y < 8; y++) {
        uint8_t row_bits = font8x8_basic[idx][y];
        for (int x = 0; x < 8; x++) {
             /* Check bit 7-x (since MSB is left-most pixel) */
             int val = (row_bits >> (7-x)) & 1;
             
             if (val) {
                 fb_put_pixel(cur_x + x, cur_y + y, 0x00FFFFFF); /* White text */
             } else {
                 /* Transparent/Black background */
                 /* Optionally clear background if we want opaque text box */
                 /* fb_put_pixel(cur_x + x, cur_y + y, 0x00000000); */
             }
        }
    }
    
    cur_x += 9; /* 8 pixels + 1 spacing */
}
