/*
 * pwd.c - Print working directory
 *
 * Unix V6 x86 Port
 * Built-in shell command to display current working directory.
 */

#include "../../kernel/include/types.h"
#include "../../kernel/include/param.h"
#include "../../kernel/include/user.h"
#include "../../kernel/include/inode.h"

/* External dependencies */
extern void printf(const char *fmt, ...);
extern void readi(struct inode *ip);
extern struct inode *iget(dev_t dev, ino_t ino);
extern void iput(struct inode *ip);

extern struct user u;
extern struct inode *rootdir;

/* V6 Directory Entry Structure */
struct v6_direct {
    uint16_t d_ino;
    char     d_name[14];
};

#define MAX_PATH_DEPTH 16

/*
 * find_name_in_dir - Find the name of an inode in a directory
 * Returns 1 if found, 0 if not found
 */
static int find_name_in_dir(struct inode *dir, ino_t target_ino, char *name_out) {
    struct v6_direct entry;
    uint32_t dir_size;
    
    dir_size = ((uint32_t)(dir->i_size0 & 0xFF) << 16) | dir->i_size1;
    
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    
    while (u.u_offset[1] < dir_size) {
        u.u_base = (char *)&entry;
        u.u_count = sizeof(struct v6_direct);
        u.u_segflg = 1;
        u.u_error = 0;
        
        readi(dir);
        
        if (u.u_error || u.u_count != 0) {
            break;
        }
        
        if (entry.d_ino == target_ino) {
            /* Skip . and .. */
            if (entry.d_name[0] == '.' && 
                (entry.d_name[1] == '\0' || 
                 (entry.d_name[1] == '.' && entry.d_name[2] == '\0'))) {
                continue;
            }
            /* Copy name */
            int i;
            for (i = 0; i < 14 && entry.d_name[i]; i++) {
                name_out[i] = entry.d_name[i];
            }
            name_out[i] = '\0';
            return 1;
        }
    }
    return 0;
}

/*
 * get_parent_ino - Get the inode number of '..' in a directory
 */
static ino_t get_parent_ino(struct inode *dir) {
    struct v6_direct entry;
    uint32_t dir_size;
    
    dir_size = ((uint32_t)(dir->i_size0 & 0xFF) << 16) | dir->i_size1;
    
    u.u_offset[0] = 0;
    u.u_offset[1] = 0;
    
    while (u.u_offset[1] < dir_size) {
        u.u_base = (char *)&entry;
        u.u_count = sizeof(struct v6_direct);
        u.u_segflg = 1;
        u.u_error = 0;
        
        readi(dir);
        
        if (u.u_error || u.u_count != 0) {
            break;
        }
        
        if (entry.d_name[0] == '.' && entry.d_name[1] == '.' && entry.d_name[2] == '\0') {
            return entry.d_ino;
        }
    }
    return 0;
}

/*
 * builtin_pwd - Print current working directory
 */
void builtin_pwd(void) {
    char path_components[MAX_PATH_DEPTH][15];
    int depth = 0;
    struct inode *cur;
    ino_t cur_ino, parent_ino;
    dev_t dev;
    int i;
    
    if (u.u_cdir == NULL) {
        printf("(no current directory)\n");
        return;
    }
    
    /* Check if we're at root */
    if (u.u_cdir == rootdir || u.u_cdir->i_number == 1) {
        printf("/\n");
        return;
    }
    
    dev = u.u_cdir->i_dev;
    cur_ino = u.u_cdir->i_number;
    
    /* Walk up the tree to root */
    while (cur_ino != 1 && depth < MAX_PATH_DEPTH) {
        /* Get parent inode */
        cur = iget(dev, cur_ino);
        if (cur == NULL) break;
        
        parent_ino = get_parent_ino(cur);
        iput(cur);
        
        if (parent_ino == 0) break;
        
        /* Get parent directory */
        struct inode *parent = iget(dev, parent_ino);
        if (parent == NULL) break;
        
        /* Find our name in parent */
        if (find_name_in_dir(parent, cur_ino, path_components[depth])) {
            depth++;
        }
        iput(parent);
        
        cur_ino = parent_ino;
    }
    
    /* Print path */
    if (depth == 0) {
        printf("/\n");
    } else {
        for (i = depth - 1; i >= 0; i--) {
            printf("/%s", path_components[i]);
        }
        printf("\n");
    }
}
