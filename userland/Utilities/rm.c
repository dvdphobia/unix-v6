/* rm.c - Delete files */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

extern void perror(const char *s);
int rmdir(const char *path);

#include <dirent.h>
#include <string.h>

void delete_recursive(const char *path) {
    DIR *d = opendir(path);
    if (!d) return;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char subpath[256];
        snprintf(subpath, sizeof(subpath), "%s/%s", path, de->d_name);
        
        struct stat st;
        if (stat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            delete_recursive(subpath);
            rmdir(subpath);
        } else {
            unlink(subpath);
        }
    }
    closedir(d);
}

int main(int argc, char **argv) {
    int recursive = 0;
    int i;
    
    if (argc < 2) {
        printf("usage: rm [-r] file...\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-R") == 0) {
            recursive = 1;
            continue;
        }

        struct stat st;
        if (lstat(argv[i], &st) < 0) {
            perror("rm");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (recursive) {
                delete_recursive(argv[i]);
                if (rmdir(argv[i]) < 0) perror("rmdir");
            } else {
                printf("rm: cannot remove '%s': Is a directory\n", argv[i]);
            }
        } else {
            if (unlink(argv[i]) < 0) perror("rm");
        }
    }
    return 0;
}
