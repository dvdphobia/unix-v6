/* dirent.c - Directory operations */
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

/* Unix V6 on-disk directory entry structure */
struct v6_direct {
    unsigned short d_ino;     /* Inode number (2 bytes) */
    char d_name[14];          /* Filename (14 bytes) */
};

/* O_DIRECTORY might not be defined in V6 headers yet */
#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif

DIR *opendir(const char *name) {
    int fd = open(name, O_RDONLY | O_DIRECTORY);
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
    
    struct v6_direct *entry;
    
    while (1) {
        /* If buffer needs refilling */
        if (dirp->buf_pos >= dirp->buf_len) {
            /* Read chunks of V6 entries into the buffer */
            /* We treat dirp->buf as raw bytes from disk */
            dirp->buf_len = read(dirp->fd, dirp->buf, sizeof(dirp->buf));
            if (dirp->buf_len <= 0)
                return NULL;
            dirp->buf_pos = 0;
            
            /* Ensure we read a multiple of entry size */
            /* If not, ignore partial entry at end? Or handle it?
             * Standard read usually returns block aligned. */
        }
        
        /* Check boundary for safety */
        if ((dirp->buf_pos + sizeof(struct v6_direct)) > (unsigned int)dirp->buf_len) {
            /* Remaining bytes not enough for an entry? Unlikely if block aligned.
             * Force read next block. */
             dirp->buf_pos = dirp->buf_len;
             continue;
        }

        /* Get next V6 entry pointer */
        entry = (struct v6_direct *)(dirp->buf + dirp->buf_pos);
        dirp->buf_pos += sizeof(struct v6_direct);
        
        /* Skip deleted entries (inode 0) */
        if (entry->d_ino == 0)
            continue;
            
        /* Convert V6 entry to POSIX dirent */
        dirp->entry.d_ino = entry->d_ino;
        dirp->entry.d_off = 0; /* TODO: Maintain offsets? */
        dirp->entry.d_reclen = sizeof(struct dirent);
        dirp->entry.d_type = DT_UNKNOWN; /* V6 doesn't store type in dir */
        
        /* Copy name (up to 14 chars, might not be null-terminated in V6 if full) */
        int i;
        for (i = 0; i < 14; i++) {
            if (entry->d_name[i] == '\0') break;
            dirp->entry.d_name[i] = entry->d_name[i];
        }
        dirp->entry.d_name[i] = '\0';
        
        return &dirp->entry;
    }
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
        lseek(dirp->fd, 0, SEEK_SET);
        dirp->buf_pos = 0;
        dirp->buf_len = 0;
    }
}

long telldir(DIR *dirp) {
    if (!dirp) return -1;
    /* This is approximate since we buffer */
    return lseek(dirp->fd, 0, SEEK_CUR) - dirp->buf_len + dirp->buf_pos;
}

void seekdir(DIR *dirp, long loc) {
    if (dirp) {
        lseek(dirp->fd, loc, SEEK_SET);
        dirp->buf_pos = 0; /* Invalidate buffer */
        dirp->buf_len = 0;
    }
}
