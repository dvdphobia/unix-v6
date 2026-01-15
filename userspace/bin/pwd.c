#include "../syscalls.h"

int main(void) {
    write(1, "/\n", 2);
    return 0;
}
