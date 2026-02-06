#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

void perror(const char *s);

static int is_octal(const char *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '7') return 0;
        s++;
    }
    return 1;
}

static int oct_to_int(const char *s) {
    int v = 0;
    while (*s) {
        v = (v << 3) + (*s - '0');
        s++;
    }
    return v;
}

int main(int argc, char **argv) {
    int mode = 0755;
    const char *path = NULL;
    int i;

    if (argc < 2) {
        printf("usage: mkdir [-m mode] directory\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'm' && argv[i][2] == '\0') {
                if (i + 1 < argc && is_octal(argv[i+1])) {
                    mode = oct_to_int(argv[i+1]);
                    i++; /* skip mode arg */
                } else {
                    printf("mkdir: invalid mode\n");
                    return 1;
                }
            } else {
                printf("mkdir: unknown option %s\n", argv[i]);
                return 1;
            }
        } else {
            path = argv[i];
        }
    }

    if (!path) {
        printf("usage: mkdir [-m mode] directory\n");
        return 1;
    }

    if (mkdir(path, (mode_t)mode) < 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}
