/* ps.c - Process status */

#include "../syscalls.h"

int main(void) {
    /* Simple stub - V6 doesn't have easy process enumeration from userland */
    write(1, "PID  STAT  CMD\n", 15);
    write(1, "1    S     init\n", 16);
    write(1, "2    R     sh\n", 14);
    write(1, "\nNote: Full ps requires kernel support\n", 39);
    return 0;
}
