/* cp.c - Copy files */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 4096

int main(int argc, char **argv) {
    int src_fd, dest_fd;
    ssize_t n_read, n_written;
    char buf[BUFSIZE];
    struct stat st;

    if (argc != 3) {
        printf("usage: cp source_file dest_file\n");
        return 1;
    }

    /* Open source file */
    src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        printf("cp: cannot open source file '%s'\n", argv[1]);
        return 1;
    }

    /* Check if source is a directory */
    if (fstat(src_fd, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("cp: omitting directory '%s'\n", argv[1]);
        close(src_fd);
        return 1;
    }

    /* Open/Create destination file */
    /* Mode 0666 (rw-rw-rw-) adjusted by umask */
    dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd < 0) {
        printf("cp: cannot create destination file '%s'\n", argv[2]);
        close(src_fd);
        return 1;
    }

    /* Copy loop */
    while ((n_read = read(src_fd, buf, BUFSIZE)) > 0) {
        char *ptr = buf;
        ssize_t remaining = n_read;
        
        while (remaining > 0) {
            n_written = write(dest_fd, ptr, remaining);
            if (n_written < 0) {
                printf("cp: write error to '%s'\n", argv[2]);
                close(src_fd);
                close(dest_fd);
                return 1;
            }
            ptr += n_written;
            remaining -= n_written;
        }
    }

    if (n_read < 0) {
        printf("cp: read error from '%s'\n", argv[1]);
    }

    close(src_fd);
    close(dest_fd);
    return 0;
}
