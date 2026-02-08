### Simple GUI Application Kit

To create a GUI application, you need to:

1. Include `<sys/fb.h>`
2. Call `getfbinfo(&fb)` to get the framebuffer address.
3. Draw directly to the screen!

#### Examples

**Basic Drawing:**
```c
#include <stdio.h>
#include <sys/fb.h>

int main() {
    struct fb_info fb;
    if (getfbinfo(&fb) < 0) return 1;
    
    // Draw a red pixel at (100, 100)
    uint32_t *pixel = (uint32_t*)((uint8_t*)fb.addr + 100 * fb.pitch + 100 * 4);
    *pixel = 0x00FF0000;
    
    return 0;
}
```

#### WindowServer
The system includes a basic WindowServer (`/sbin/winserver`) that demonstrates a desktop environment. 
Source: `userland/WindowServer/main.c`.
