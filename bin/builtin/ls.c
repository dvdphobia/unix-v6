/*
 * ls.c - List directory contents
 *
 * Unix V6 x86 Port
 * Built-in shell command to list directory entries.
 */

#include "../../kernel/include/types.h"
#include "../../kernel/include/param.h"
#include "../../kernel/include/user.h"
#include "../../kernel/include/inode.h"

/* External dependencies */
extern void printf(const char *fmt, ...);
extern void readi(struct inode *ip);
extern struct inode *namei(int (*func)(), int flag);
/* iget is declared in inode.h */
extern void iput(struct inode *ip);

extern struct user u;

/* V6 Directory Entry Structure */
struct v6_direct {
    uint16_t d_ino;         /* Inode number */
    char     d_name[14];    /* File name (max 14 chars) */
};

/* Helper to read path from u.u_dirp */
static int path_reader(void) {
    char c = *((char *)u.u_dirp);
    u.u_dirp = (char *)u.u_dirp + 1;
    return (int)(unsigned char)c;
}

/*
 * builtin_ls - List directory contents
 *
 * If path is NULL, lists current directory.
 * Otherwise lists the specified path.
 */
void builtin_ls(const char *args) {
    struct inode *dir;
    struct v6_direct entry;
    int count = 0;
    uint32_t dir_size;
    int long_listing = 0;
    char path_buf[128];
    const char *path = NULL;

    /* Parse args */
    if (args) {
        const char *p = args;
        while (*p) {
            /* Skip leading spaces */
            while (*p == ' ') p++;
            if (*p == '\0') break;

            if (p[0] == '-' && p[1] == 'l') {
                long_listing = 1;
                p += 2;
                continue;
            } else {
                /* Assume this is the path */
                int i = 0;
                while (*p && *p != ' ' && i < 127) {
                    path_buf[i++] = *p++;
                }
                path_buf[i] = 0;
                path = path_buf;
                /* Ignore rest of args for now */
                break;
            }
        }
    }
    
    /* Determine which directory to list */
    if (path == NULL || path[0] == '\0') {
        /* Use current directory */
        dir = u.u_cdir;
        if (dir == NULL) {
            printf("ls: no current directory\n");
            return;
        }
        dir->i_count++;  /* Bump reference since we'll use it */
    } else {
        /* Lookup the specified path */
        u.u_dirp = (caddr_t)path;
        dir = namei(path_reader, 0);
        if (dir == NULL) {
            printf("ls: %s: No such file or directory\n", path);
            return;
        }
    }
    
    /* Verify it's a directory */
    if ((dir->i_mode & IFMT) != IFDIR) {
        /* It's a file, show info for it */
        if (long_listing) {
            char name[15];
            const char *p = path; 
            const char *last_slash = path;
            while (*p) {
                if (*p == '/') last_slash = p + 1;
                p++;
            }
            int k = 0; 
            while (*last_slash && k < 14) name[k++] = *last_slash++;
            name[k] = 0;

            struct inode *ip = dir;
            
            if ((ip->i_mode & IFMT) == IFDIR) printf("d");
            else if ((ip->i_mode & IFMT) == IFCHR) printf("c");
            else if ((ip->i_mode & IFMT) == IFBLK) printf("b");
            else printf("-");
            
            printf("%c%c%c",
                (ip->i_mode & 0400) ? 'r' : '-',
                (ip->i_mode & 0200) ? 'w' : '-',
                (ip->i_mode & 0100) ? 'x' : '-');
            printf("%c%c%c",
                (ip->i_mode & 0040) ? 'r' : '-',
                (ip->i_mode & 0020) ? 'w' : '-',
                (ip->i_mode & 0010) ? 'x' : '-');
            printf("%c%c%c",
                (ip->i_mode & 0004) ? 'r' : '-',
                (ip->i_mode & 0002) ? 'w' : '-',
                (ip->i_mode & 0001) ? 'x' : '-');
                
            printf(" %2d %4d %s\n", 
                ip->i_nlink, 
                ((uint32_t)(ip->i_size0&0xff)<<16)|ip->i_size1, 
                name);
        } else {
            printf("%s\n", path);
        }
        iput(dir);
        return;
    }
    
    /* Get directory size */
    dir_size = ((uint32_t)(dir->i_size0 & 0xFF) << 16) | dir->i_size1;
    
    /* Read directory entries directly from inode data blocks */
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    
    while ((uint32_t)u.u_offset[1] < dir_size) {
        u.u_base = (char *)&entry;
        u.u_count = sizeof(struct v6_direct);
        u.u_segflg = 1;  /* Kernel segment */
        u.u_error = 0;
        
        readi(dir);
        
        /* Check for error or incomplete read */
        if (u.u_error) {
            break;
        }
        
        /* If we didn't read a full entry, we're done */
        if (u.u_count != 0) {
            break;
        }
        
        /* Skip empty entries (deleted files) */
        if (entry.d_ino != 0) {
            /* Safely print name (max 14 chars) */
            char name[15];
            int i;
            for (i = 0; i < 14 && entry.d_name[i]; i++) {
                name[i] = entry.d_name[i];
            }
            name[i] = '\0';
            
            if (long_listing) {
                struct inode *ip;
                int is_self = (entry.d_ino == dir->i_number);

                if (is_self) {
                    ip = dir;
                } else {
                    ip = iget(dir->i_dev, entry.d_ino);
                }

                if (ip) {
                    if ((ip->i_mode & IFMT) == IFDIR) printf("d");
                    else if ((ip->i_mode & IFMT) == IFCHR) printf("c");
                    else if ((ip->i_mode & IFMT) == IFBLK) printf("b");
                    else printf("-");
                    
                    printf("%c%c%c",
                        (ip->i_mode & 0400) ? 'r' : '-',
                        (ip->i_mode & 0200) ? 'w' : '-',
                        (ip->i_mode & 0100) ? 'x' : '-');
                    printf("%c%c%c",
                        (ip->i_mode & 0040) ? 'r' : '-',
                        (ip->i_mode & 0020) ? 'w' : '-',
                        (ip->i_mode & 0010) ? 'x' : '-');
                    printf("%c%c%c",
                        (ip->i_mode & 0004) ? 'r' : '-',
                        (ip->i_mode & 0002) ? 'w' : '-',
                        (ip->i_mode & 0001) ? 'x' : '-');
                        
                    printf(" %2d %4d %s\n", 
                        ip->i_nlink, 
                        ((uint32_t)(ip->i_size0&0xff)<<16)|ip->i_size1, 
                        name);
                    
                    if (!is_self) {
                        iput(ip);
                    }
                } else {
                    printf("?---------    ?    ? %s\n", name);
                }
            } else {
                printf("%s  ", name);
                count++;
                
                /* Newline every 5 entries for readability */
                if (count % 5 == 0) {
                    printf("\n");
                }
            }
        }
    }
    
    if (!long_listing) {
        if (count > 0 && count % 5 != 0) {
            printf("\n");
        } else if (count == 0) {
            printf("(empty)\n");
        }
    }
    
    /* Release directory inode */
    iput(dir);
}
