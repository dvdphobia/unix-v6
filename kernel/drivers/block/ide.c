/* ide.c - Minimal ATA PIO block driver (primary master)
 * Provides a simple persistent block device for QEMU.
 */

#include "include/types.h"
#include "include/param.h"
#include "include/buf.h"
#include "include/conf.h"
#include "include/systm.h"
#include "include/user.h"

/* I/O port accessors */
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);
extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t val);

/* ATA primary I/O ports */
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCNT      0x1F2
#define ATA_LBA0        0x1F3
#define ATA_LBA1        0x1F4
#define ATA_LBA2        0x1F5
#define ATA_HDDEVSEL    0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

/* Status bits */
#define ATA_SR_BSY      0x80
#define ATA_SR_DRQ      0x08
#define ATA_SR_ERR      0x01

/* Commands */
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

#define IDE_MAJOR       1

static struct devtab ide_tab;
static int ide_present;

static int ide_wait_ready(void) {
    for (int i = 0; i < 100000; i++) {
        uint8_t st = inb(ATA_STATUS);
        if ((st & ATA_SR_BSY) == 0) {
            return 0;
        }
    }
    return -1;
}

static int ide_wait_drq(void) {
    for (int i = 0; i < 100000; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st & ATA_SR_ERR) {
            return -1;
        }
        if ((st & ATA_SR_BSY) == 0 && (st & ATA_SR_DRQ)) {
            return 0;
        }
    }
    return -1;
}

static int ide_select_lba(uint32_t lba) {
    if (ide_wait_ready() != 0) {
        return -1;
    }
    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCNT, 1);
    outb(ATA_LBA0, lba & 0xFF);
    outb(ATA_LBA1, (lba >> 8) & 0xFF);
    outb(ATA_LBA2, (lba >> 16) & 0xFF);
    return 0;
}

static int ide_open(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return ide_present ? 0 : -1;
}

static int ide_close(dev_t dev, int flag) {
    (void)dev;
    (void)flag;
    return 0;
}

static int ide_strategy(struct buf *bp) {
    uint32_t lba = bp->b_blkno;
    int count = (-bp->b_wcount) * 2;
    char *addr = bp->b_addr;

    if (!ide_present) {
        bp->b_flags |= B_ERROR;
        bp->b_error = ENXIO;
        iodone(bp);
        return -1;
    }

    if (count <= 0 || (count % BSIZE) != 0) {
        bp->b_flags |= B_ERROR;
        bp->b_error = EINVAL;
        iodone(bp);
        return -1;
    }

    for (int off = 0; off < count; off += BSIZE, lba++) {
        if (ide_select_lba(lba) != 0) {
            bp->b_flags |= B_ERROR;
            bp->b_error = EIO;
            break;
        }

        if (bp->b_flags & B_READ) {
            outb(ATA_COMMAND, ATA_CMD_READ);
            if (ide_wait_drq() != 0) {
                bp->b_flags |= B_ERROR;
                bp->b_error = EIO;
                break;
            }
            for (int i = 0; i < BSIZE / 2; i++) {
                uint16_t w = inw(ATA_DATA);
                addr[off + i * 2] = w & 0xFF;
                addr[off + i * 2 + 1] = (w >> 8) & 0xFF;
            }
        } else {
            outb(ATA_COMMAND, ATA_CMD_WRITE);
            if (ide_wait_drq() != 0) {
                bp->b_flags |= B_ERROR;
                bp->b_error = EIO;
                break;
            }
            for (int i = 0; i < BSIZE / 2; i++) {
                uint16_t w = (uint16_t)(uint8_t)addr[off + i * 2] |
                             ((uint16_t)(uint8_t)addr[off + i * 2 + 1] << 8);
                outw(ATA_DATA, w);
            }
        }
    }

    bp->b_resid = 0;
    iodone(bp);
    return 0;
}

struct bdevsw ide_bdevsw = {
    .d_open = ide_open,
    .d_close = ide_close,
    .d_strategy = ide_strategy,
    .d_tab = &ide_tab,
};

void ide_init(void) {
    extern struct bdevsw bdevsw[];
    extern int nblkdev;

    uint8_t st = inb(ATA_STATUS);
    if (st == 0x00 || st == 0xFF) {
        ide_present = 0;
        return;
    }

    ide_present = 1;
    bdevsw[IDE_MAJOR] = ide_bdevsw;
    if (nblkdev <= IDE_MAJOR) {
        nblkdev = IDE_MAJOR + 1;
    }
}
