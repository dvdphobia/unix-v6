#include "../syscalls.h"
#include "../libc/fcntl.h"
#include "../libc/unistd.h"

struct v6_direct {
    unsigned short d_ino;
    char d_name[14];
};

static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void write_str(const char *s) {
    write(1, s, str_len(s));
}

int main(int argc, char **argv) {
    const char *path = ".";
    int fd;
    struct v6_direct ent;
    int first = 1;

    if (argc > 1 && argv[1] && argv[1][0]) {
        path = argv[1];
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        write_str("ls: cannot open\n");
        return 1;
    }

    while (read(fd, &ent, sizeof(ent)) == sizeof(ent)) {
        if (ent.d_ino == 0) {
            continue;
        }
        if (!first) {
            write_str(" ");
        }
        ent.d_name[13] = '\0';
        write_str(ent.d_name);
        first = 0;
    }
    write_str("\n");
    close(fd);
    return 0;
}
