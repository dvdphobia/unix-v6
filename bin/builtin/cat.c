/*
 * cat.c - Concatenate and display files
 *
 * Unix V6 x86 Port
 * Built-in shell command to display file contents.
 */

#include "../../kernel/include/types.h"
#include "../../kernel/include/param.h"
#include "../../kernel/include/user.h"
#include "../../kernel/include/inode.h"

/* External dependencies */
extern void printf(const char *fmt, ...);
extern void readi(struct inode *ip);
extern struct inode *namei(int (*func)(), int flag);
extern void iput(struct inode *ip);

extern struct user u;

/* Helper to read path from u.u_dirp */
static int path_reader(void) {
    char c = *((char *)u.u_dirp);
    u.u_dirp = (char *)u.u_dirp + 1;
    return (int)(unsigned char)c;
}

/*
 * builtin_cat - Display file contents
 */
void builtin_cat(const char *path) {
    struct inode *ip;
    char buf[64];
    uint32_t file_size;
    
    if (path == NULL || path[0] == '\0') {
        printf("usage: cat <file>\n");
        return;
    }
    
    /* Lookup the file */
    u.u_dirp = (caddr_t)path;
    ip = namei(path_reader, 0);
    
    if (ip == NULL) {
        printf("cat: %s: No such file or directory\n", path);
        return;
    }
    
    /* Check if it's a regular file */
    if ((ip->i_mode & IFMT) == IFDIR) {
        printf("cat: %s: Is a directory\n", path);
        iput(ip);
        return;
    }
    
    /* Get file size */
    file_size = ((uint32_t)(ip->i_size0 & 0xFF) << 16) | ip->i_size1;
    
    if (file_size == 0) {
        iput(ip);
        return;
    }
    
    /* Read and print file contents */
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    
    while (u.u_offset[1] < file_size) {
        int to_read = sizeof(buf) - 1;
        if (file_size - u.u_offset[1] < (uint32_t)to_read) {
            to_read = file_size - u.u_offset[1];
        }
        
        u.u_base = buf;
        u.u_count = to_read;
        u.u_segflg = 1;  /* Kernel segment */
        u.u_error = 0;
        
        readi(ip);
        
        if (u.u_error) {
            printf("\ncat: read error\n");
            break;
        }
        
        /* Calculate how many bytes were actually read */
        int nread = to_read - u.u_count;
        if (nread <= 0) break;
        
        /* Print the bytes */
        for (int i = 0; i < nread; i++) {
            char c = buf[i];
            if (c == '\n' || (c >= 32 && c < 127)) {
                printf("%c", c);
            } else if (c == '\t') {
                printf("    ");
            }
        }
    }
    
    printf("\n");
    iput(ip);
}
