/* file.h - Open File Structure
 * Unix V6 x86 Port
 */

#ifndef _UNIX_FILE_H
#define _UNIX_FILE_H

#include <unix/types.h>

struct inode;

/*
 * Open file structure - one per open file
 */
struct file {
    int8_t      f_flag;         /* File flags */
    int8_t      f_count;        /* Reference count */
    struct inode *f_inode;      /* Pointer to inode */
    off_t       f_offset;       /* Read/write offset */
};

/* File flags */
#define FREAD   0x01            /* Open for reading */
#define FWRITE  0x02            /* Open for writing */
#define FPIPE   0x04            /* Pipe */

/* Function prototypes */
struct file *getf(int fd);
struct file *falloc(void);
void closef(struct file *fp);

extern struct file file[];

#endif /* _UNIX_FILE_H */
