#include "syscalls.h"

static void print_motd(void) {
    int fd = open("/etc/motd", O_RDONLY);
    if (fd < 0) {
        return;
    }
    char buf[128];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }
    close(fd);
}

int main() {
    int pid;
    char *argv[] = {"sh", 0};

    print_motd();
    
    while(1) {
        pid = fork();
        if (pid == 0) {
            exec("/bin/sh", argv);
            exit(1);
        }
        while(wait(0) != -1);
    }
    return 0;
}
