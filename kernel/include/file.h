/* file.h - Unix V6 x86 Port File Structure
 * Ported from original V6 file.h for PDP-11
 * Open file table entries
 */

#ifndef _FILE_H_
#define _FILE_H_

#include "param.h"
#include "types.h"

/* Forward declaration */
struct inode;

/*
 * One file structure is allocated for each open/creat/pipe call.
 * Main use is to hold the read/write pointer associated with each
 * open file.
 */
struct file {
    int8_t      f_flag;         /* File flags */
    int8_t      f_count;        /* Reference count */
    struct inode *f_inode;      /* Pointer to inode structure */
    off_t       f_offset[2];    /* Read/write character pointer (64-bit) */
};

/* File flags */
#define FREAD       01          /* File open for reading */
#define FWRITE      02          /* File open for writing */
#define FPIPE       04          /* File is a pipe */
#define FAPPEND     010         /* Append on each write */
#define FNONBLOCK   020         /* Non-blocking I/O */

/* Global file table */
extern struct file file[NFILE];

/*
 * File function prototypes
 */
struct file *falloc(void);
struct file *getf(int fd);
void closef(struct file *fp);

#endif /* _FILE_H_ */
