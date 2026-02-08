#include <fcntl.h>
#include <unistd.h>

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
    
    write(1, "INIT: Starting...\n", 18);

    print_motd();
    
    while(1) {
        pid = fork();
        if (pid == 0) {
            execv("/bin/sh", argv);
            write(1, "exec failed\n", 13);
            _exit(1);
        }
        while(wait(0) != -1);
    }
    return 0;
}
