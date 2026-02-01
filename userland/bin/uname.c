/* uname.c - Print system information */

#include "../syscalls.h"

int main(void) {
    write(1, "Unix V6 x86 (i686)\n", 19);
    return 0;
}
