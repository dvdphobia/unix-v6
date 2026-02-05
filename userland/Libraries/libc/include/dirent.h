/* dirent.h - Directory operations */
#ifndef _DIRENT_H
#define _DIRENT_H

/* Directory entry structure */
struct dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

/* Directory stream type */
typedef struct {
    int fd;
    struct dirent entry;
    char buf[1024];
    int buf_pos;
    int buf_len;
} DIR;

/* Directory operations */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);
long telldir(DIR *dirp);
void seekdir(DIR *dirp, long loc);

/* File types for d_type */
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14

#endif /* _DIRENT_H */
