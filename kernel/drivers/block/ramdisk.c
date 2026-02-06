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
#define RAMDISK_SIZE    (8 * 1024 * 1024)
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

static void rd_write_data_inode(struct rd_inode *ip, const uint8_t *data, uint32_t bytes, int *data_block) {
    int blocks = (bytes + BSIZE - 1) / BSIZE;
    int j;

    ip->i_size0 = (uint8_t)((bytes >> 16) & 0xFF);
    ip->i_size1 = (uint16_t)(bytes & 0xFFFF);
    for (j = 0; j < 8; j++) {
        ip->i_addr[j] = 0;
    }

    if (blocks <= 8) {
        for (j = 0; j < blocks; j++) {
            ip->i_addr[j] = (*data_block)++;
            rd_write_block(ip->i_addr[j], data + (j * BSIZE), bytes - (j * BSIZE));
        }
        return;
    }

    ip->i_mode |= ILARG;

    int bn = 0;
    for (j = 0; j < 7 && bn < blocks; j++) {
        uint32_t indir[NINDIR];
        int i;
        for (i = 0; i < NINDIR; i++) indir[i] = 0;
        for (i = 0; i < NINDIR && bn < blocks; i++) {
            indir[i] = (*data_block)++;
            rd_write_block(indir[i], data + (bn * BSIZE), bytes - (bn * BSIZE));
            bn++;
        }
        ip->i_addr[j] = (*data_block)++;
        rd_write_block(ip->i_addr[j], (const uint8_t *)indir, sizeof(indir));
    }

    if (bn < blocks) {
        uint32_t dindir[NINDIR];
        int di;
        for (di = 0; di < NINDIR; di++) dindir[di] = 0;
        for (di = 0; di < NINDIR && bn < blocks; di++) {
            uint32_t indir[NINDIR];
            int i;
            for (i = 0; i < NINDIR; i++) indir[i] = 0;
            for (i = 0; i < NINDIR && bn < blocks; i++) {
                indir[i] = (*data_block)++;
                rd_write_block(indir[i], data + (bn * BSIZE), bytes - (bn * BSIZE));
                bn++;
            }
            dindir[di] = (*data_block)++;
            rd_write_block(dindir[di], (const uint8_t *)indir, sizeof(indir));
        }
        ip->i_addr[7] = (*data_block)++;
        rd_write_block(ip->i_addr[7], (const uint8_t *)dindir, sizeof(dindir));
    }
}

static void rd_write_dir_inode(struct rd_inode *ip, struct v6_direct *ents, int count, int *data_block) {
    int bytes = count * (int)sizeof(struct v6_direct);
    rd_write_data_inode(ip, (const uint8_t *)ents, (uint32_t)bytes, data_block);
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
    static struct v6_direct include_dir[256];
    static struct v6_direct lib_dir[64];
    static struct v6_direct inc_arpa_dir[64];
    static struct v6_direct inc_bits_dir[64];
    static struct v6_direct inc_net_dir[64];
    static struct v6_direct inc_netinet_dir[64];
    static struct v6_direct inc_netpacket_dir[64];
    static struct v6_direct inc_scsi_dir[64];
    static struct v6_direct inc_sys_dir[64];
    int root_cnt = 0, etc_cnt = 0, bin_cnt = 0, dev_cnt = 0, usr_cnt = 0;
    int include_cnt = 0, lib_cnt = 0;
    int inc_arpa_cnt = 0, inc_bits_cnt = 0, inc_net_cnt = 0, inc_netinet_cnt = 0;
    int inc_netpacket_cnt = 0, inc_scsi_cnt = 0, inc_sys_cnt = 0;
    int file_count = 0;
    int i;

    const int ino_root = 1;
    const int ino_etc = 2;
    const int ino_bin = 3;
    const int ino_dev = 4;
    const int ino_usr = 5;
    const int ino_console = 6;
    const int ino_include = 7;
    const int ino_lib = 8;
    const int ino_inc_arpa = 9;
    const int ino_inc_bits = 10;
    const int ino_inc_net = 11;
    const int ino_inc_netinet = 12;
    const int ino_inc_netpacket = 13;
    const int ino_inc_scsi = 14;
    const int ino_inc_sys = 15;
    int next_ino = 16;

    for (i = 0; rd_files[i].path; i++) {
        file_count++;
    }

    /* Size inode table to fit directories + files */
    int dir_inodes = 15;
    int total_inodes = dir_inodes + file_count;
    int inode_bytes = total_inodes * (int)sizeof(struct rd_inode);
    int inode_blocks = (inode_bytes + BSIZE - 1) / BSIZE;
    int data_block = 2 + inode_blocks;

    fp->s_isize = inode_blocks;
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
    for (i = 0; i < 256; i++) {
        include_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) include_dir[i].d_name[j] = 0;
    }
    for (i = 0; i < 64; i++) {
        lib_dir[i].d_ino = 0;
        inc_arpa_dir[i].d_ino = 0;
        inc_bits_dir[i].d_ino = 0;
        inc_net_dir[i].d_ino = 0;
        inc_netinet_dir[i].d_ino = 0;
        inc_netpacket_dir[i].d_ino = 0;
        inc_scsi_dir[i].d_ino = 0;
        inc_sys_dir[i].d_ino = 0;
        for (int j = 0; j < 14; j++) {
            lib_dir[i].d_name[j] = 0;
            inc_arpa_dir[i].d_name[j] = 0;
            inc_bits_dir[i].d_name[j] = 0;
            inc_net_dir[i].d_name[j] = 0;
            inc_netinet_dir[i].d_name[j] = 0;
            inc_netpacket_dir[i].d_name[j] = 0;
            inc_scsi_dir[i].d_name[j] = 0;
            inc_sys_dir[i].d_name[j] = 0;
        }
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
    root_dir[root_cnt].d_ino = ino_include;
    root_dir[root_cnt].d_name[0] = 'i';
    root_dir[root_cnt].d_name[1] = 'n';
    root_dir[root_cnt].d_name[2] = 'c';
    root_dir[root_cnt].d_name[3] = 'l';
    root_dir[root_cnt].d_name[4] = 'u';
    root_dir[root_cnt].d_name[5] = 'd';
    root_dir[root_cnt].d_name[6] = 'e';
    root_dir[root_cnt].d_name[7] = 0;
    root_cnt++;
    root_dir[root_cnt].d_ino = ino_lib;
    root_dir[root_cnt].d_name[0] = 'l';
    root_dir[root_cnt].d_name[1] = 'i';
    root_dir[root_cnt].d_name[2] = 'b';
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

    dip[ino_usr - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_usr - 1].i_nlink = 2;
    dip[ino_include - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_include - 1].i_nlink = 2;
    dip[ino_lib - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_lib - 1].i_nlink = 2;
    dip[ino_inc_arpa - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_arpa - 1].i_nlink = 2;
    dip[ino_inc_bits - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_bits - 1].i_nlink = 2;
    dip[ino_inc_net - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_net - 1].i_nlink = 2;
    dip[ino_inc_netinet - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_netinet - 1].i_nlink = 2;
    dip[ino_inc_netpacket - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_netpacket - 1].i_nlink = 2;
    dip[ino_inc_scsi - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_scsi - 1].i_nlink = 2;
    dip[ino_inc_sys - 1].i_mode = IALLOC | IFDIR | 0755;
    dip[ino_inc_sys - 1].i_nlink = 2;

    dip[ino_console - 1].i_mode = IALLOC | IFCHR | 0666;
    dip[ino_console - 1].i_nlink = 1;
    dip[ino_console - 1].i_addr[0] = 0;

    for (i = 0; i < 14; i++) include_dir[include_cnt].d_name[i] = 0;
    include_dir[include_cnt].d_ino = ino_include;
    include_dir[include_cnt].d_name[0] = '.';
    include_cnt++;
    include_dir[include_cnt].d_ino = ino_root;
    include_dir[include_cnt].d_name[0] = '.';
    include_dir[include_cnt].d_name[1] = '.';
    include_cnt++;

    for (i = 0; i < 14; i++) lib_dir[lib_cnt].d_name[i] = 0;
    lib_dir[lib_cnt].d_ino = ino_lib;
    lib_dir[lib_cnt].d_name[0] = '.';
    lib_cnt++;
    lib_dir[lib_cnt].d_ino = ino_root;
    lib_dir[lib_cnt].d_name[0] = '.';
    lib_dir[lib_cnt].d_name[1] = '.';
    lib_cnt++;

    inc_arpa_dir[inc_arpa_cnt].d_ino = ino_inc_arpa;
    inc_arpa_dir[inc_arpa_cnt].d_name[0] = '.';
    inc_arpa_cnt++;
    inc_arpa_dir[inc_arpa_cnt].d_ino = ino_include;
    inc_arpa_dir[inc_arpa_cnt].d_name[0] = '.';
    inc_arpa_dir[inc_arpa_cnt].d_name[1] = '.';
    inc_arpa_cnt++;

    inc_bits_dir[inc_bits_cnt].d_ino = ino_inc_bits;
    inc_bits_dir[inc_bits_cnt].d_name[0] = '.';
    inc_bits_cnt++;
    inc_bits_dir[inc_bits_cnt].d_ino = ino_include;
    inc_bits_dir[inc_bits_cnt].d_name[0] = '.';
    inc_bits_dir[inc_bits_cnt].d_name[1] = '.';
    inc_bits_cnt++;

    inc_net_dir[inc_net_cnt].d_ino = ino_inc_net;
    inc_net_dir[inc_net_cnt].d_name[0] = '.';
    inc_net_cnt++;
    inc_net_dir[inc_net_cnt].d_ino = ino_include;
    inc_net_dir[inc_net_cnt].d_name[0] = '.';
    inc_net_dir[inc_net_cnt].d_name[1] = '.';
    inc_net_cnt++;

    inc_netinet_dir[inc_netinet_cnt].d_ino = ino_inc_netinet;
    inc_netinet_dir[inc_netinet_cnt].d_name[0] = '.';
    inc_netinet_cnt++;
    inc_netinet_dir[inc_netinet_cnt].d_ino = ino_include;
    inc_netinet_dir[inc_netinet_cnt].d_name[0] = '.';
    inc_netinet_dir[inc_netinet_cnt].d_name[1] = '.';
    inc_netinet_cnt++;

    inc_netpacket_dir[inc_netpacket_cnt].d_ino = ino_inc_netpacket;
    inc_netpacket_dir[inc_netpacket_cnt].d_name[0] = '.';
    inc_netpacket_cnt++;
    inc_netpacket_dir[inc_netpacket_cnt].d_ino = ino_include;
    inc_netpacket_dir[inc_netpacket_cnt].d_name[0] = '.';
    inc_netpacket_dir[inc_netpacket_cnt].d_name[1] = '.';
    inc_netpacket_cnt++;

    inc_scsi_dir[inc_scsi_cnt].d_ino = ino_inc_scsi;
    inc_scsi_dir[inc_scsi_cnt].d_name[0] = '.';
    inc_scsi_cnt++;
    inc_scsi_dir[inc_scsi_cnt].d_ino = ino_include;
    inc_scsi_dir[inc_scsi_cnt].d_name[0] = '.';
    inc_scsi_dir[inc_scsi_cnt].d_name[1] = '.';
    inc_scsi_cnt++;

    inc_sys_dir[inc_sys_cnt].d_ino = ino_inc_sys;
    inc_sys_dir[inc_sys_cnt].d_name[0] = '.';
    inc_sys_cnt++;
    inc_sys_dir[inc_sys_cnt].d_ino = ino_include;
    inc_sys_dir[inc_sys_cnt].d_name[0] = '.';
    inc_sys_dir[inc_sys_cnt].d_name[1] = '.';
    inc_sys_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_arpa;
    include_dir[include_cnt].d_name[0] = 'a';
    include_dir[include_cnt].d_name[1] = 'r';
    include_dir[include_cnt].d_name[2] = 'p';
    include_dir[include_cnt].d_name[3] = 'a';
    include_dir[include_cnt].d_name[4] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_bits;
    include_dir[include_cnt].d_name[0] = 'b';
    include_dir[include_cnt].d_name[1] = 'i';
    include_dir[include_cnt].d_name[2] = 't';
    include_dir[include_cnt].d_name[3] = 's';
    include_dir[include_cnt].d_name[4] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_net;
    include_dir[include_cnt].d_name[0] = 'n';
    include_dir[include_cnt].d_name[1] = 'e';
    include_dir[include_cnt].d_name[2] = 't';
    include_dir[include_cnt].d_name[3] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_netinet;
    include_dir[include_cnt].d_name[0] = 'n';
    include_dir[include_cnt].d_name[1] = 'e';
    include_dir[include_cnt].d_name[2] = 't';
    include_dir[include_cnt].d_name[3] = 'i';
    include_dir[include_cnt].d_name[4] = 'n';
    include_dir[include_cnt].d_name[5] = 'e';
    include_dir[include_cnt].d_name[6] = 't';
    include_dir[include_cnt].d_name[7] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_netpacket;
    include_dir[include_cnt].d_name[0] = 'n';
    include_dir[include_cnt].d_name[1] = 'e';
    include_dir[include_cnt].d_name[2] = 't';
    include_dir[include_cnt].d_name[3] = 'p';
    include_dir[include_cnt].d_name[4] = 'a';
    include_dir[include_cnt].d_name[5] = 'c';
    include_dir[include_cnt].d_name[6] = 'k';
    include_dir[include_cnt].d_name[7] = 'e';
    include_dir[include_cnt].d_name[8] = 't';
    include_dir[include_cnt].d_name[9] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_scsi;
    include_dir[include_cnt].d_name[0] = 's';
    include_dir[include_cnt].d_name[1] = 'c';
    include_dir[include_cnt].d_name[2] = 's';
    include_dir[include_cnt].d_name[3] = 'i';
    include_dir[include_cnt].d_name[4] = 0;
    include_cnt++;

    include_dir[include_cnt].d_ino = ino_inc_sys;
    include_dir[include_cnt].d_name[0] = 's';
    include_dir[include_cnt].d_name[1] = 'y';
    include_dir[include_cnt].d_name[2] = 's';
    include_dir[include_cnt].d_name[3] = 0;
    include_cnt++;

    for (i = 0; rd_files[i].path; i++) {
        const struct rd_file *rf = &rd_files[i];
        struct rd_inode *ip = &dip[next_ino - 1];
        char name[15];
        int j;

        ip->i_mode = IALLOC | (rf->mode & 07777);
        ip->i_nlink = 1;
        ip->i_size0 = (uint8_t)((rf->size >> 16) & 0xFF);
        ip->i_size1 = (uint16_t)(rf->size & 0xFFFF);
        for (j = 0; j < 8; j++) {
            ip->i_addr[j] = 0;
        }

        rd_write_data_inode(ip, rf->data, rf->size, &data_block);

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
        } else if (rf->path[0] == '/' && rf->path[1] == 'l' && rf->path[2] == 'i' && rf->path[3] == 'b' && rf->path[4] == '/') {
            lib_dir[lib_cnt].d_ino = next_ino;
            for (j = 0; j < 14; j++) lib_dir[lib_cnt].d_name[j] = 0;
            for (j = 0; j < 14; j++) {
                lib_dir[lib_cnt].d_name[j] = name[j];
                if (name[j] == 0) break;
            }
            lib_cnt++;
        } else if (rf->path[0] == '/' && rf->path[1] == 'i' && rf->path[2] == 'n' && rf->path[3] == 'c' && rf->path[4] == 'l' &&
                   rf->path[5] == 'u' && rf->path[6] == 'd' && rf->path[7] == 'e' && rf->path[8] == '/') {
            const char *sub = rf->path + 9;
            const char *slash = 0;
            for (const char *p = sub; *p; p++) {
                if (*p == '/') { slash = p; break; }
            }
            if (!slash) {
                include_dir[include_cnt].d_ino = next_ino;
                for (j = 0; j < 14; j++) include_dir[include_cnt].d_name[j] = 0;
                for (j = 0; j < 14; j++) {
                    include_dir[include_cnt].d_name[j] = name[j];
                    if (name[j] == 0) break;
                }
                include_cnt++;
            } else {
                const char *subdir = sub;
                if (subdir[0] == 'a' && subdir[1] == 'r' && subdir[2] == 'p' && subdir[3] == 'a' && subdir[4] == '/') {
                    inc_arpa_dir[inc_arpa_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_arpa_dir[inc_arpa_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_arpa_dir[inc_arpa_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_arpa_cnt++;
                } else if (subdir[0] == 'b' && subdir[1] == 'i' && subdir[2] == 't' && subdir[3] == 's' && subdir[4] == '/') {
                    inc_bits_dir[inc_bits_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_bits_dir[inc_bits_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_bits_dir[inc_bits_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_bits_cnt++;
                } else if (subdir[0] == 'n' && subdir[1] == 'e' && subdir[2] == 't' && subdir[3] == '/') {
                    inc_net_dir[inc_net_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_net_dir[inc_net_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_net_dir[inc_net_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_net_cnt++;
                } else if (subdir[0] == 'n' && subdir[1] == 'e' && subdir[2] == 't' && subdir[3] == 'i' &&
                           subdir[4] == 'n' && subdir[5] == 'e' && subdir[6] == 't' && subdir[7] == '/') {
                    inc_netinet_dir[inc_netinet_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_netinet_dir[inc_netinet_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_netinet_dir[inc_netinet_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_netinet_cnt++;
                } else if (subdir[0] == 'n' && subdir[1] == 'e' && subdir[2] == 't' && subdir[3] == 'p' &&
                           subdir[4] == 'a' && subdir[5] == 'c' && subdir[6] == 'k' && subdir[7] == 'e' &&
                           subdir[8] == 't' && subdir[9] == '/') {
                    inc_netpacket_dir[inc_netpacket_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_netpacket_dir[inc_netpacket_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_netpacket_dir[inc_netpacket_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_netpacket_cnt++;
                } else if (subdir[0] == 's' && subdir[1] == 'c' && subdir[2] == 's' && subdir[3] == 'i' && subdir[4] == '/') {
                    inc_scsi_dir[inc_scsi_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_scsi_dir[inc_scsi_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_scsi_dir[inc_scsi_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_scsi_cnt++;
                } else if (subdir[0] == 's' && subdir[1] == 'y' && subdir[2] == 's' && subdir[3] == '/') {
                    inc_sys_dir[inc_sys_cnt].d_ino = next_ino;
                    for (j = 0; j < 14; j++) inc_sys_dir[inc_sys_cnt].d_name[j] = 0;
                    for (j = 0; j < 14; j++) {
                        inc_sys_dir[inc_sys_cnt].d_name[j] = name[j];
                        if (name[j] == 0) break;
                    }
                    inc_sys_cnt++;
                }
            }
        }

        next_ino++;
    }

    rd_write_dir_inode(&dip[ino_root - 1], root_dir, root_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_etc - 1], etc_dir, etc_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_bin - 1], bin_dir, bin_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_dev - 1], dev_dir, dev_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_usr - 1], usr_dir, usr_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_include - 1], include_dir, include_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_lib - 1], lib_dir, lib_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_arpa - 1], inc_arpa_dir, inc_arpa_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_bits - 1], inc_bits_dir, inc_bits_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_net - 1], inc_net_dir, inc_net_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_netinet - 1], inc_netinet_dir, inc_netinet_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_netpacket - 1], inc_netpacket_dir, inc_netpacket_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_scsi - 1], inc_scsi_dir, inc_scsi_cnt, &data_block);
    rd_write_dir_inode(&dip[ino_inc_sys - 1], inc_sys_dir, inc_sys_cnt, &data_block);

    dip[ino_etc - 1].i_size0 = 0;
    dip[ino_etc - 1].i_size1 = (uint16_t)(etc_cnt * sizeof(struct v6_direct));
    dip[ino_bin - 1].i_size0 = 0;
    dip[ino_bin - 1].i_size1 = (uint16_t)(bin_cnt * sizeof(struct v6_direct));
    rd_write_dir(dip[ino_etc - 1].i_addr[0], etc_dir, etc_cnt);
    rd_write_dir(dip[ino_bin - 1].i_addr[0], bin_dir, bin_cnt);

    int nfree = 0;
    for (daddr_t b = data_block; b < RAMDISK_BLOCKS && nfree < NICFREE; b++) {
        fp->s_free[nfree++] = b;
    }
    fp->s_nfree = nfree;
}