#include "../syscalls.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    write(1, "program template\n", 17);
    return 0;
}
