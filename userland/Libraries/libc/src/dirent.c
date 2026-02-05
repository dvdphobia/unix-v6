/* dirent.c - Directory operations */
#include <stddef.h>

extern int open(const char *path, int mode);
extern int close(int fd);
extern int read(int fd, void *buf, int count);
extern void *malloc(size_t size);
extern void free(void *ptr);

#define O_RDONLY 0x00
#define O_DIRECTORY 0x10000

/* Directory entry structure (matches kernel) */
struct dirent {
    unsigned long d_ino;      /* Inode number */
    unsigned long d_off;      /* Offset to next dirent */
    unsigned short d_reclen;  /* Length of this record */
    unsigned char d_type;     /* Type of file */
    char d_name[256];         /* Filename */
};

/* DIR structure */
typedef struct {
    int fd;
    struct dirent entry;
    char buf[1024];
    int buf_pos;
    int buf_len;
} DIR;

DIR *opendir(const char *name) {
    int fd = open(name, O_RDONLY);
    if (fd < 0)
        return NULL;
    
    DIR *dirp = malloc(sizeof(DIR));
    if (!dirp) {
        close(fd);
        return NULL;
    }
    
    dirp->fd = fd;
    dirp->buf_pos = 0;
    dirp->buf_len = 0;
    
    return dirp;
}

struct dirent *readdir(DIR *dirp) {
    if (!dirp)
        return NULL;
    
    /* Read more data if buffer is empty */
    if (dirp->buf_pos >= dirp->buf_len) {
        dirp->buf_len = read(dirp->fd, dirp->buf, sizeof(dirp->buf));
        if (dirp->buf_len <= 0)
            return NULL;
        dirp->buf_pos = 0;
    }
    
    /* Parse directory entry from buffer */
    struct dirent *src = (struct dirent *)(dirp->buf + dirp->buf_pos);
    
    /* Copy to the internal entry */
    dirp->entry.d_ino = src->d_ino;
    dirp->entry.d_off = src->d_off;
    dirp->entry.d_reclen = src->d_reclen;
    dirp->entry.d_type = src->d_type;
    
    /* Copy name */
    int i = 0;
    while (src->d_name[i] && i < 255) {
        dirp->entry.d_name[i] = src->d_name[i];
        i++;
    }
    dirp->entry.d_name[i] = '\0';
    
    /* Move to next entry */
    if (src->d_reclen > 0)
        dirp->buf_pos += src->d_reclen;
    else
        dirp->buf_pos = dirp->buf_len; /* Force read on next call */
    
    return &dirp->entry;
}

int closedir(DIR *dirp) {
    if (!dirp)
        return -1;
    
    int ret = close(dirp->fd);
    free(dirp);
    return ret;
}

void rewinddir(DIR *dirp) {
    if (dirp) {
        extern long lseek(int fd, long offset, int whence);
        lseek(dirp->fd, 0, 0); /* SEEK_SET */
        dirp->buf_pos = 0;
        dirp->buf_len = 0;
    }
}

long telldir(DIR *dirp) {
    if (!dirp)
        return -1;
    
    return dirp->entry.d_off;
}

void seekdir(DIR *dirp, long loc) {
    if (dirp) {
        extern long lseek(int fd, long offset, int whence);
        lseek(dirp->fd, loc, 0); /* SEEK_SET */
        dirp->buf_pos = 0;
        dirp->buf_len = 0;
    }
}
