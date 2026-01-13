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
#include "include/user.h"

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

/* Forward declaration */
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
 * rd_mkfs - Create minimal V6 filesystem on RAM disk
 */
static void rd_mkfs(void) {
    struct filsys *fp;
    
    /* Block 0: boot block (unused) */
    /* Block 1: superblock */
    fp = (struct filsys *)(ramdisk + BSIZE);
    
    fp->s_isize = 16;           /* 16 blocks for inodes */
    fp->s_fsize = RAMDISK_BLOCKS;
    fp->s_nfree = 1;
    fp->s_free[0] = 20;         /* First free data block (17,18,19 used) */
    fp->s_ninode = 1;
    fp->s_inode[0] = 4;         /* First free inode */
    fp->s_flock = 0;
    fp->s_ilock = 0;
    fp->s_fmod = 0;
    fp->s_ronly = 0;
    fp->s_time[0] = 0;
    fp->s_time[1] = 0;
    
    /* Setup free block list */
    int nfree = 0;
    for (daddr_t b = 20; b < RAMDISK_BLOCKS && nfree < 100; b++) {
        fp->s_free[nfree++] = b;
    }
    fp->s_nfree = nfree;
    
    /* Block 2-17: inode blocks */
    /* Create root directory inode (inode 1) */
    struct {
        uint16_t i_mode;
        uint8_t  i_nlink;
        uint8_t  i_uid;
        uint8_t  i_gid;
        uint8_t  i_size0;
        uint16_t i_size1;
        uint16_t i_addr[8];
        uint16_t i_atime[2];
        uint16_t i_mtime[2];
    } *dip = (void *)(ramdisk + 2 * BSIZE);
    
    /* Root directory - inode 1 (inode 0 is not used) */
    dip[1].i_mode = 0040755;    /* Directory, rwxr-xr-x */
    dip[1].i_nlink = 3;         /* ., .., etc */
    dip[1].i_uid = 0;
    dip[1].i_gid = 0;
    dip[1].i_size0 = 0;
    dip[1].i_size1 = 32;        /* 2 directory entries (wait, needs etc) */
    dip[1].i_addr[0] = 17;      /* First data block */
    
    /* Create /etc directory - inode 2 */
    dip[2].i_mode = 0040755;
    dip[2].i_nlink = 2;
    dip[2].i_uid = 0;
    dip[2].i_gid = 0;
    dip[2].i_size0 = 0;
    dip[2].i_size1 = 48;        /* 3 entries: ., .., init */
    dip[2].i_addr[0] = 18;

    /* Create /etc/init binary - inode 3 */
    dip[3].i_mode = 0100755;    /* Regular file, rwxr-xr-x */
    dip[3].i_nlink = 1;
    dip[3].i_uid = 0;
    dip[3].i_gid = 0;
    dip[3].i_size0 = 0;
    
    /* Tiny Init Binary (x86) */
    /* Prints "Hello, Unix V6\n" and loops */
    static const uint8_t init_bin[] = {
        0xb8, 0x04, 0x00, 0x00, 0x00,  /* mov eax, 4 (write) */
        0xbb, 0x01, 0x00, 0x00, 0x00,  /* mov ebx, 1 (stdout) */
        0xb9, 0x18, 0x00, 0x00, 0x00,  /* mov ecx, 24 (msg offset) */
        0xba, 0x0f, 0x00, 0x00, 0x00,  /* mov edx, 15 (len) */
        0xcd, 0x80,                    /* int 0x80 */
        0xeb, 0xe8,                    /* jmp -24 (loop) */
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'U', 'n', 'i', 'x', ' ', 'V', '6', 0x0a
    };
    
    dip[3].i_size1 = sizeof(init_bin);
    dip[3].i_addr[0] = 19;

    /* Root directory data block (17) */
    struct {
        uint16_t d_ino;
        char d_name[14];
    } *dep = (void *)(ramdisk + 17 * BSIZE);
    
    /* "." entry */
    dep[0].d_ino = 1;
    dep[0].d_name[0] = '.'; dep[0].d_name[1] = '\0';
    
    /* ".." entry */
    dep[1].d_ino = 1;
    dep[1].d_name[0] = '.'; dep[1].d_name[1] = '.'; dep[1].d_name[2] = '\0';
    
    /* "etc" entry */
    dep[2].d_ino = 2;
    dep[2].d_name[0] = 'e'; dep[2].d_name[1] = 't'; dep[2].d_name[2] = 'c'; dep[2].d_name[3] = '\0';
    
    /* /etc directory data block (18) */
    dep = (void *)(ramdisk + 18 * BSIZE);
    
    /* "." entry */
    dep[0].d_ino = 2;
    dep[0].d_name[0] = '.'; dep[0].d_name[1] = '\0';
    
    /* ".." entry */
    dep[1].d_ino = 1;
    dep[1].d_name[0] = '.'; dep[1].d_name[1] = '.'; dep[1].d_name[2] = '\0';
    
    /* "init" entry */
    dep[2].d_ino = 3;
    dep[2].d_name[0] = 'i'; dep[2].d_name[1] = 'n'; dep[2].d_name[2] = 'i'; dep[2].d_name[3] = 't'; dep[2].d_name[4] = '\0';

    /* /etc/init file content (19) */
    char *file_data = (void *)(ramdisk + 19 * BSIZE);
    for (unsigned int i=0; i < sizeof(init_bin); i++) {
        file_data[i] = init_bin[i];
    }
    
    /* /dev/console special inode (5) -> already set i_addr[0]=0 (0,0) above */
}
