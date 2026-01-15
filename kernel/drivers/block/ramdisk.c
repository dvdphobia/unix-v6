/* ramdisk.c - RAM Disk Block Device Driver
 * Unix V6 x86 Port
 * 
 * Provides a simple RAM-based block device for testing
 * without requiring real hardware disk drivers.
 */

#include "include/types.h"
#include "include/param.h"
#include "include/buf.h"
#include "include/conf.h"
#include "include/filsys.h"
#include "include/inode.h"
#include "include/user.h"
#include "drivers/block/ramdisk_files.h"

/* Helper functions */
/* None needed currently */

/* RAM disk size: 1MB */
#define RAMDISK_SIZE    (1024 * 1024)
#define RAMDISK_BLOCKS  (RAMDISK_SIZE / BSIZE)

/* RAM disk storage */
static char ramdisk[RAMDISK_SIZE];

/* Device major number */
#define RAMDISK_MAJOR   0

/* Device table for ramdisk */
static struct devtab rd_tab;

/* Forward declarations */
static int rd_open(dev_t dev, int flag);
static int rd_close(dev_t dev, int flag);
static int rd_strategy(struct buf *bp);

/* RAM disk device operations */
struct bdevsw rd_bdevsw = {
    .d_open = rd_open,
    .d_close = rd_close,
    .d_strategy = rd_strategy,
    .d_tab = &rd_tab,
};

/*
 * rd_open - Open RAM disk device
 */
static int rd_open(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

/*
 * rd_close - Close RAM disk device
 */
static int rd_close(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

/*
 * rd_strategy - Perform I/O on RAM disk
 */
static int rd_strategy(struct buf *bp) {
    daddr_t blkno = bp->b_blkno;
    char *addr = bp->b_addr;
    int count = (-bp->b_wcount) * 2;
    
    /* Check bounds */
    if (blkno < 0 || blkno >= RAMDISK_BLOCKS) {
        bp->b_flags |= B_ERROR;
        bp->b_error = ENXIO;
        iodone(bp);
        return -1;
    }
    
    /* Limit transfer size */
    if (blkno + (count / BSIZE) > RAMDISK_BLOCKS) {
        count = (RAMDISK_BLOCKS - blkno) * BSIZE;
    }
    
    /* Perform transfer */
    char *diskaddr = ramdisk + (blkno * BSIZE);
    
    if (bp->b_flags & B_READ) {
        /* Read from RAM disk */
        for (int i = 0; i < count; i++) {
            addr[i] = diskaddr[i];
        }
    } else {
        /* Write to RAM disk */
        for (int i = 0; i < count; i++) {
            diskaddr[i] = addr[i];
        }
    }
    
    bp->b_resid = 0;
    iodone(bp);
    return 0;
}

/* Minimal on-disk inode layout for rd_mkfs */
struct rd_inode {
    uint16_t i_mode;
    uint8_t  i_nlink;
    uint8_t  i_uid;
    uint8_t  i_gid;
    uint8_t  i_size0;
    uint16_t i_size1;
    uint16_t i_addr[8];
    uint16_t i_atime[2];
    uint16_t i_mtime[2];
};

struct v6_direct {
    uint16_t d_ino;
    char d_name[14];
};

static void rd_write_block(daddr_t blkno, const uint8_t *data, uint32_t size) {
    char *dst = ramdisk + (blkno * BSIZE);
    uint32_t i;

    for (i = 0; i < BSIZE; i++) {
        dst[i] = 0;
    }
    for (i = 0; i < size && i < BSIZE; i++) {
        dst[i] = (char)data[i];
    }
}

static void rd_write_dir(daddr_t blkno, struct v6_direct *ents, int count) {
    rd_write_block(blkno, (const uint8_t *)ents, count * sizeof(struct v6_direct));
}

static int rd_name_from_path(const char *path, char *out) {
    const char *p = path;
    const char *last = path;
    int i = 0;
    while (*p) {
        if (*p == '/') {
            last = p + 1;
        }
        p++;
    }
    while (*last && i < 14) {
        out[i++] = *last++;
    }
    out[i] = 0;
    return i;
}

static void rd_mkfs(void);

/*
 * rd_init - Initialize RAM disk with a minimal filesystem
 */
void rd_init(void) {
    extern void printf(const char *fmt, ...);
    extern struct bdevsw bdevsw[];
    extern int nblkdev;
    
    printf("ramdisk: initializing %d KB RAM disk\n", RAMDISK_SIZE / 1024);
    
    /* Clear RAM disk */
    for (int i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }
    
    /* Register RAM disk as block device 0 */
    bdevsw[RAMDISK_MAJOR] = rd_bdevsw;
    nblkdev = 1;
    
    /* Create a minimal V6 filesystem on the RAM disk */
    rd_mkfs();
    
    printf("ramdisk: ready, %d blocks available\n", RAMDISK_BLOCKS);
}

/*
 * rd_mkfs - Create a minimal V6 filesystem on RAM disk
 */
static void rd_mkfs(void) {
    struct filsys *fp = (struct filsys *)(ramdisk + BSIZE);
    struct rd_inode *dip = (void *)(ramdisk + 2 * BSIZE);
    struct v6_direct root_dir[16];
    struct v6_direct etc_dir[16];
    struct v6_direct bin_dir[32];
    struct v6_direct dev_dir[8];
    struct v6_direct usr_dir[8];
    int root_cnt = 0, etc_cnt = 0, bin_cnt = 0, dev_cnt = 0, usr_cnt = 0;
    int file_count = 0;
    int i;

    const int ino_root = 1;
    const int ino_etc = 2;
    const int ino_bin = 3;
    const int ino_dev = 4;
    const int ino_usr = 5;
    const int ino_console = 6;
    int next_ino = 7;

    for (i = 0; rd_files[i].path; i++) {
        file_count++;
    }

    /* V6 inode layout is fixed: blocks 2-16 (15 blocks). */
    int data_block = 17;

    fp->s_isize = 15;
    fp->s_fsize = RAMDISK_BLOCKS;
    fp->s_nfree = 0;
    fp->s_ninode = 0;
    fp->s_flock = 0;
    fp->s_ilock = 0;
    fp->s_fmod = 0;
    fp->s_ronly = 0;
    fp->s_time[0] = 0;
    fp->s_time[1] = 0;

    for (i = 0; i < 16; i++) {
        root_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) root_dir[i].d_name[j] = 0;
        etc_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) etc_dir[i].d_name[j] = 0;
    }
    for (i = 0; i < 32; i++) {
        bin_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) bin_dir[i].d_name[j] = 0;
    }
    for (i = 0; i < 8; i++) {
        dev_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) dev_dir[i].d_name[j] = 0;
        usr_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) usr_dir[i].d_name[j] = 0;
    }

    root_dir[root_cnt].d_ino = ino_root;
    root_dir[root_cnt].d_name[0] = '.';
    root_dir[root_cnt].d_name[1] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_root;
    root_dir[root_cnt].d_name[0] = '.';
    root_dir[root_cnt].d_name[1] = '.';
    root_dir[root_cnt].d_name[2] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_etc;
    root_dir[root_cnt].d_name[0] = 'e';
    root_dir[root_cnt].d_name[1] = 't';
    root_dir[root_cnt].d_name[2] = 'c';
    root_dir[root_cnt].d_name[3] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_bin;
    root_dir[root_cnt].d_name[0] = 'b';
    root_dir[root_cnt].d_name[1] = 'i';
    root_dir[root_cnt].d_name[2] = 'n';
    root_dir[root_cnt].d_name[3] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_dev;
    root_dir[root_cnt].d_name[0] = 'd';
    root_dir[root_cnt].d_name[1] = 'e';
    root_dir[root_cnt].d_name[2] = 'v';
    root_dir[root_cnt].d_name[3] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_usr;
    root_dir[root_cnt].d_name[0] = 'u';
    root_dir[root_cnt].d_name[1] = 's';
    root_dir[root_cnt].d_name[2] = 'r';
    root_dir[root_cnt].d_name[3] = 0;
    root_cnt++;

    etc_dir[etc_cnt].d_ino = ino_etc;
    etc_dir[etc_cnt].d_name[0] = '.';
    etc_dir[etc_cnt].d_name[1] = 0;
    etc_cnt++;
    etc_dir[etc_cnt].d_ino = ino_root;
    etc_dir[etc_cnt].d_name[0] = '.';
    etc_dir[etc_cnt].d_name[1] = '.';
    etc_dir[etc_cnt].d_name[2] = 0;
    etc_cnt++;

    bin_dir[bin_cnt].d_ino = ino_bin;
    bin_dir[bin_cnt].d_name[0] = '.';
    bin_dir[bin_cnt].d_name[1] = 0;
    bin_cnt++;
    bin_dir[bin_cnt].d_ino = ino_root;
    bin_dir[bin_cnt].d_name[0] = '.';
    bin_dir[bin_cnt].d_name[1] = '.';
    bin_dir[bin_cnt].d_name[2] = 0;
    bin_cnt++;

    dev_dir[dev_cnt].d_ino = ino_dev;
    dev_dir[dev_cnt].d_name[0] = '.';
    dev_dir[dev_cnt].d_name[1] = 0;
    dev_cnt++;
    dev_dir[dev_cnt].d_ino = ino_root;
    dev_dir[dev_cnt].d_name[0] = '.';
    dev_dir[dev_cnt].d_name[1] = '.';
    dev_dir[dev_cnt].d_name[2] = 0;
    dev_cnt++;
    dev_dir[dev_cnt].d_ino = ino_console;
    dev_dir[dev_cnt].d_name[0] = 'c';
    dev_dir[dev_cnt].d_name[1] = 'o';
    dev_dir[dev_cnt].d_name[2] = 'n';
    dev_dir[dev_cnt].d_name[3] = 's';
    dev_dir[dev_cnt].d_name[4] = 'o';
    dev_dir[dev_cnt].d_name[5] = 'l';
    dev_dir[dev_cnt].d_name[6] = 'e';
    dev_dir[dev_cnt].d_name[7] = 0;
    dev_cnt++;

    usr_dir[usr_cnt].d_ino = ino_usr;
    usr_dir[usr_cnt].d_name[0] = '.';
    usr_dir[usr_cnt].d_name[1] = 0;
    usr_cnt++;
    usr_dir[usr_cnt].d_ino = ino_root;
    usr_dir[usr_cnt].d_name[0] = '.';
    usr_dir[usr_cnt].d_name[1] = '.';
    usr_dir[usr_cnt].d_name[2] = 0;
    usr_cnt++;

    dip[ino_root - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_root - 1].i_nlink = 6;
    dip[ino_root - 1].i_size0 = 0;
    dip[ino_root - 1].i_size1 = (uint16_t)(root_cnt * sizeof(struct v6_direct));

    dip[ino_etc - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_etc - 1].i_nlink = 2;
    dip[ino_etc - 1].i_size0 = 0;
    dip[ino_etc - 1].i_size1 = (uint16_t)(etc_cnt * sizeof(struct v6_direct));

    dip[ino_bin - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_bin - 1].i_nlink = 2;
    dip[ino_bin - 1].i_size0 = 0;
    dip[ino_bin - 1].i_size1 = (uint16_t)(bin_cnt * sizeof(struct v6_direct));

    dip[ino_dev - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_dev - 1].i_nlink = 2;
    dip[ino_dev - 1].i_size0 = 0;
    dip[ino_dev - 1].i_size1 = (uint16_t)(dev_cnt * sizeof(struct v6_direct));

    dip[ino_usr - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_usr - 1].i_nlink = 2;
    dip[ino_usr - 1].i_size0 = 0;
    dip[ino_usr - 1].i_size1 = (uint16_t)(usr_cnt * sizeof(struct v6_direct));

    dip[ino_console - 1].i_mode = IALLOC | IFCHR | 0666;
    dip[ino_console - 1].i_nlink = 1;
    dip[ino_console - 1].i_addr[0] = 0;

    dip[ino_root - 1].i_addr[0] = data_block++;
    dip[ino_etc - 1].i_addr[0] = data_block++;
    dip[ino_bin - 1].i_addr[0] = data_block++;
    dip[ino_dev - 1].i_addr[0] = data_block++;
    dip[ino_usr - 1].i_addr[0] = data_block++;

    rd_write_dir(dip[ino_root - 1].i_addr[0], root_dir, root_cnt);
    rd_write_dir(dip[ino_etc - 1].i_addr[0], etc_dir, etc_cnt);
    rd_write_dir(dip[ino_bin - 1].i_addr[0], bin_dir, bin_cnt);
    rd_write_dir(dip[ino_dev - 1].i_addr[0], dev_dir, dev_cnt);
    rd_write_dir(dip[ino_usr - 1].i_addr[0], usr_dir, usr_cnt);

    for (i = 0; rd_files[i].path; i++) {
        const struct rd_file *rf = &rd_files[i];
        struct rd_inode *ip = &dip[next_ino - 1];
        char name[15];
        int blocks = (rf->size + BSIZE - 1) / BSIZE;
        int j;

        ip->i_mode = IALLOC | (rf->mode & 07777);
        ip->i_nlink = 1;
        ip->i_size0 = (uint8_t)((rf->size >> 16) & 0xFF);
        ip->i_size1 = (uint16_t)(rf->size & 0xFFFF);

        for (j = 0; j < blocks && j < 8; j++) {
            ip->i_addr[j] = data_block++;
            rd_write_block(ip->i_addr[j], rf->data + (j * BSIZE), rf->size - (j * BSIZE));
        }

        rd_name_from_path(rf->path, name);

        if (rf->path[0] == '/' && rf->path[1] == 'e' && rf->path[2] == 't' && rf->path[3] == 'c' && rf->path[4] == '/') {
            etc_dir[etc_cnt].d_ino = next_ino;
            for (j = 0; j < 14; j++) etc_dir[etc_cnt].d_name[j] = 0;
            for (j = 0; j < 14; j++) {
                etc_dir[etc_cnt].d_name[j] = name[j];
                if (name[j] == 0) break;
            }
            etc_cnt++;
        } else if (rf->path[0] == '/' && rf->path[1] == 'b' && rf->path[2] == 'i' && rf->path[3] == 'n' && rf->path[4] == '/') {
            bin_dir[bin_cnt].d_ino = next_ino;
            for (j = 0; j < 14; j++) bin_dir[bin_cnt].d_name[j] = 0;
            for (j = 0; j < 14; j++) {
                bin_dir[bin_cnt].d_name[j] = name[j];
                if (name[j] == 0) break;
            }
            bin_cnt++;
        }

        next_ino++;
    }

    dip[ino_etc - 1].i_size0 = 0;
    dip[ino_etc - 1].i_size1 = (uint16_t)(etc_cnt * sizeof(struct v6_direct));
    dip[ino_bin - 1].i_size0 = 0;
    dip[ino_bin - 1].i_size1 = (uint16_t)(bin_cnt * sizeof(struct v6_direct));
    rd_write_dir(dip[ino_etc - 1].i_addr[0], etc_dir, etc_cnt);
    rd_write_dir(dip[ino_bin - 1].i_addr[0], bin_dir, bin_cnt);

    int nfree = 0;
    for (daddr_t b = data_block; b < RAMDISK_BLOCKS && nfree < 100; b++) {
        fp->s_free[nfree++] = b;
    }
    fp->s_nfree = nfree;
}
