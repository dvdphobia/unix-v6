#include "../syscalls.h"
#include "../libc/fcntl.h"
#include "../libc/unistd.h"

static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void write_str(const char *s) {
    write(1, s, str_len(s));
}

static void cat_fd(int fd) {
    char buf[128];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }
}

int main(int argc, char **argv) {
    int i;

    if (argc < 2) {
        cat_fd(0);
        return 0;
    }

    for (i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            write_str("cat: cannot open\n");
            continue;
        }
        cat_fd(fd);
        close(fd);
    }
    return 0;
}
