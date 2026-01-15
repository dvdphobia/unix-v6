#include "../syscalls.h"

static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int main(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        write(1, argv[i], str_len(argv[i]));
        if (i + 1 < argc) {
            write(1, " ", 1);
        }
    }
    write(1, "\n", 1);
    return 0;
}
