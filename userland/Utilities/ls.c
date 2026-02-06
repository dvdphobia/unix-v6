#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

struct v6_direct {
    ino_t d_ino;
    char d_name[14];
};

int flag_long = 0;

void print_mode(mode_t m) {
    if (S_ISDIR(m)) putchar('d');
    else if (S_ISCHR(m)) putchar('c');
    else if (S_ISBLK(m)) putchar('b');
    else putchar('-');
    
    putchar(m & S_IREAD ? 'r' : '-');
    putchar(m & S_IWRITE ? 'w' : '-');
    putchar(m & S_IEXEC ? 'x' : '-');
    putchar(m & (S_IREAD >> 3) ? 'r' : '-');
    putchar(m & (S_IWRITE >> 3) ? 'w' : '-');
    putchar(m & (S_IEXEC >> 3) ? 'x' : '-');
    putchar(m & (S_IREAD >> 6) ? 'r' : '-');
    putchar(m & (S_IWRITE >> 6) ? 'w' : '-');
    putchar(m & (S_IEXEC >> 6) ? 'x' : '-');
}

void list_file(const char *path, const char *name) {
    struct stat st;
    if (stat(path, &st) < 0) {
        printf("ls: cannot stat %s\n", name);
        return;
    }
    
    if (flag_long) {
        print_mode(st.st_mode);
        printf(" %2d %4d %4d %6ld %ld %s\n", 
            st.st_nlink, st.st_uid, st.st_gid, (long)st.st_size, (long)st.st_mtime, name);
    } else {
        printf("%s\n", name);
    }
}

void list_dir(const char *path) {
    int fd;
    struct v6_direct ent;
    struct stat st;
    char fullpath[512];
    
    if (stat(path, &st) < 0) {
        printf("ls: cannot access %s\n", path);
        return;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        list_file(path, path);
        return;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("ls: cannot open %s\n", path);
        return;
    }

    while (read(fd, &ent, sizeof(ent)) == sizeof(ent)) {
        if (ent.d_ino == 0) continue;
        
        char name[15];
        int i;
        for (i = 0; i < 14; i++) name[i] = ent.d_name[i];
        name[14] = '\0';
        
        if (flag_long) {
             if (strcmp(path, "/") == 0) {
                 snprintf(fullpath, sizeof(fullpath), "/%s", name);
             } else {
                 snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);
             }
             list_file(fullpath, name);
        } else {
            printf("%s\n", name);
        }
    }
    close(fd);
}

int main(int argc, char **argv) {
    int i;
    int has_path = 0;
    
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            int j;
            for (j = 1; argv[i][j]; j++) {
                if (argv[i][j] == 'l') flag_long = 1;
            }
        } else {
            has_path = 1;
        }
    }
    
    if (!has_path) {
        list_dir(".");
    } else {
        for (i = 1; i < argc; i++) {
             if (argv[i][0] == '-') continue;
             list_dir(argv[i]);
        }
    }
    
    return 0;
}
