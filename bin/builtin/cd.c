/*
 * cd.c - Change directory
 *
 * Unix V6 x86 Port
 * Built-in shell command to change current working directory.
 */

#include "../../kernel/include/types.h"
#include "../../kernel/include/param.h"
#include "../../kernel/include/user.h"
#include "../../kernel/include/inode.h"

/* External dependencies */
extern void printf(const char *fmt, ...);
extern struct inode *namei(int (*func)(), int flag);
extern void iput(struct inode *ip);
extern void prele(struct inode *ip);

extern struct user u;

/* Helper to read path from u.u_dirp */
static int path_reader(void) {
    char c = *((char *)u.u_dirp);
    u.u_dirp = (char *)u.u_dirp + 1;
    return (int)(unsigned char)c;
}

/*
 * builtin_cd - Change current working directory
 *
 * Changes to the specified path, or prints usage if no path given.
 */
void builtin_cd(const char *path) {
    struct inode *ip;
    
    if (path == NULL || path[0] == '\0') {
        printf("usage: cd <directory>\n");
        return;
    }
    
    /* Lookup the path */
    u.u_dirp = (caddr_t)path;
    ip = namei(path_reader, 0);
    
    if (ip == NULL) {
        printf("cd: %s: No such file or directory\n", path);
        return;
    }
    
    /* Verify it's a directory */
    if ((ip->i_mode & IFMT) != IFDIR) {
        printf("cd: %s: Not a directory\n", path);
        iput(ip);
        return;
    }
    
    /* Release old current directory */
    if (u.u_cdir) {
        iput(u.u_cdir);
    }
    
    /* Set new current directory */
    u.u_cdir = ip;
    prele(ip);  /* Release lock but keep reference */
}
