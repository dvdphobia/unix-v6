/* clear.c - Clear terminal screen */

#include "../syscalls.h"
#include "../libc/fcntl.h"
#include "../libc/unistd.h"

int main(void) {
    /* ANSI escape sequence to clear screen */
    write(1, "\033[2J\033[H", 7);
    return 0;
}
