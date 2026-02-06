/* main.c - Unix V6 x86 Port Kernel Main
 * Ported from original V6 ken/main.c for PDP-11
 * Modernized for 32-bit x86 architecture
 *
 * Original authors: Ken Thompson, Dennis Ritchie
 * x86 port: Unix V6 Modernization Project
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/systm.h"
#include "include/proc.h"
#include "include/text.h"
#include "include/inode.h"
#include "include/buf.h"
#include "include/file.h"
#include "include/conf.h"
#include "include/reg.h"

/*
 * Icode is the bootstrap program executed in user mode
 * to bring up the system.
 * Execs /etc/init
 */
static uint8_t icode[] = {
    0xb8, 0x0b, 0x00, 0x00, 0x00,  /* mov eax, 11 (exec) */
    0xbb, 0x14, 0x00, 0x00, 0x00,  /* mov ebx, 20 (addr of path) */
    0xb9, 0x20, 0x00, 0x00, 0x00,  /* mov ecx, 32 (addr of argv) */
    0xcd, 0x80,                    /* int 0x80 */
    0xeb, 0xfe,                    /* jmp $ */
    0x00,                          /* padding */
    '/', 'e', 't', 'c', '/', 'i', 'n', 'i', 't', 0,
    0x00, 0x00,
    0x14, 0x00, 0x00, 0x00,        /* argv[0] */
    0x00, 0x00, 0x00, 0x00         /* argv[1] */
};

/*
 * Global kernel data structures
 */

/* Process table */
struct proc proc[NPROC];
struct proc *curproc;

/* User structure - one per process, swapped with process */
struct user u __attribute__((aligned(64)));

/* Inode table */
struct inode inode[NINODE];
struct inode *rootdir;

/* File table */
struct file file[NFILE];

/* Buffer cache */
struct buf buf[NBUF];
char buffers[NBUF][BSIZE];
struct buf bfreelist;
struct buf swbuf;

/* Text table */
/* struct text text[NTEXT]; Defined in text.c */


/* Mount table */
struct mount mount[NMOUNT];

/* Callout table */
struct callo callout[NCALL];

/* System variables */
char canonb[CANBSIZ];
uint32_t coremap[CMAPSIZ];
uint32_t swapmap[SMAPSIZ];
extern int cputype;  /* Defined in x86.S */
int execnt = 0;
int lbolt = 0;
time_t time[2] = {0, 0};
time_t tout[2] = {0, 0};
int mpid = 0;
int8_t runin = 0;
int8_t runout = 0;
int8_t runrun = 0;
int8_t curpri = 0;
uint32_t maxmem = 0;
uint32_t *lks = 0;
dev_t rootdev = 0;
dev_t swapdev = 0;
daddr_t swplo = 0;
int nswap = 0;
int updlock = 0;
blkno_t rablock = 0;

/* Device tables - minimal for now */
struct bdevsw bdevsw[NBLKDEV];
struct cdevsw cdevsw[NCHRDEV];
int nblkdev = 0;    /* Number of block devices */
int nchrdev = 0;    /* Number of character devices */

/*
 * VGA Console Output (x86 specific)
 * Replaces PDP-11 console driver
 */
#define VGA_BUFFER  ((volatile uint16_t*)0xB8000)
#define VGA_WHITE   0x0F00
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* Serial port (COM1) for terminal output */
#define COM1_PORT   0x3F8

static int vga_row = 0;
static int vga_col = 0;

/* External I/O functions from x86.S */
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern char _end[]; /* Defined in linker script */

/*
 * Initialize Programmable Interrupt Controller (PIC)
 * Remap IRQ 0-15 to ISR 32-47 to avoid conflict with CPU exceptions
 */
static void pic_init(void) {
    /* mask all interrupts */
    outb(0x21, 0xff);
    outb(0xA1, 0xff);

    /* ICW1: Init command */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    /* ICW2: Vector offsets (32 and 40) */
    outb(0x21, 0x20);  /* Master maps to 0x20 (32) */
    outb(0xA1, 0x28);  /* Slave maps to 0x28 (40) */

    /* ICW3: Cascading */
    outb(0x21, 0x04);  /* Tell Master about Slave at IRQ2 */
    outb(0xA1, 0x02);  /* Tell Slave its cascade identity */

    /* ICW4: 8086 mode */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    /* OCW1: Unmask all interrupts */
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

/* Initialize serial port COM1 */
static void serial_init(void) {
    outb(COM1_PORT + 1, 0x01);    /* Enable interrupts (Data Available only) */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB */
    outb(COM1_PORT + 0, 0x01);    /* Divisor low byte (115200 baud) */
    outb(COM1_PORT + 1, 0x00);    /* Divisor high byte */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

/* Write a character to serial port */
void serial_putchar(char c) {
    /* Wait for transmit buffer empty */
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

static void serial_puts(const char *s) {
    while (*s) {
        if (*s == '\n') serial_putchar('\r');
        serial_putchar(*s++);
    }
}

static void vga_scroll(void) {
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
    }
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    }
}

void vga_putchar(char c) {
    /* Sanity check - should never happen but prevents divide by zero */
    if (VGA_WIDTH == 0 || VGA_HEIGHT == 0) {
        return;
    }
    
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else {
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = VGA_WHITE | (uint8_t)c;
        vga_col++;
    }
    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    while (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row--;
    }
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = VGA_WHITE | ' ';
    }
    vga_row = 0;
    vga_col = 0;
}

static void vga_puts(const char *s) {
    while (*s) {
        vga_putchar(*s++);
    }
}

/*
 * Console output - writes to both VGA and serial
 */
static void console_puts(const char *s) {
    vga_puts(s);
    serial_puts(s);
}

/*
 * Simplified printf for kernel messages
 * Supports: %d, %x, %s, %c
 */
static void print_int(int n) {
    char buf[12];
    int i = 0;
    int neg = 0;
    
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    if (n == 0) {
        serial_putchar('0');
        vga_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    if (neg) {
        serial_putchar('-');
        vga_putchar('-');
    }
    while (--i >= 0) {
        serial_putchar(buf[i]);
        vga_putchar(buf[i]);
    }
}

static void print_hex(unsigned int n) {
    char buf[9];
    int i = 0;
    const char *hex = "0123456789abcdef";
    
    if (n == 0) {
        serial_putchar('0');
        vga_putchar('0');
        return;
    }
    while (n > 0) {
        buf[i++] = hex[n & 0xf];
        n >>= 4;
    }
    while (--i >= 0) {
        serial_putchar(buf[i]);
        vga_putchar(buf[i]);
    }
}

void printf(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            /* Skip width and alignment specifiers */
            int left_align = 0;
            int width = 0;
            if (*fmt == '-') {
                left_align = 1;
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
            (void)left_align; /* Not implemented yet */
            (void)width;      /* Not implemented yet */
            
            switch (*fmt) {
            case 'd':
                print_int(__builtin_va_arg(ap, int));
                break;
            case 'x':
                print_hex(__builtin_va_arg(ap, unsigned int));
                break;
            case 's': {
                const char *s = __builtin_va_arg(ap, const char *);
                if (s) console_puts(s);
                break;
            }
            case 'c':
                serial_putchar(__builtin_va_arg(ap, int));
                vga_putchar(__builtin_va_arg(ap, int));
                break;
            case '%':
                serial_putchar('%');
                vga_putchar('%');
                break;
            default:
                serial_putchar('%');
                vga_putchar('%');
                serial_putchar(*fmt);
                vga_putchar(*fmt);
                break;
            }
        } else {
            if (*fmt == '\n') serial_putchar('\r');
            serial_putchar(*fmt);
            vga_putchar(*fmt);
        }
        fmt++;
    }
    __builtin_va_end(ap);
}

/*
 * panic - Print message and halt system
 * Original V6 function from prf.c
 */
void panic(const char *msg) {
    console_puts("\n\npanic: ");
    console_puts(msg);
    console_puts("\n");
    
    /* Halt the CPU */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/*
 * prdev - Print device error message
 */
void prdev(const char *msg, dev_t dev) {
    printf("%s on dev %d/%d\n", msg, major(dev), minor(dev));
}

/*
 * Null device functions
 */
int nulldev(void) {
    return 0;
}

int nodev(void) {
    u.u_error = ENODEV;
    return -1;
}

/*
 * Memory management stubs (Removed - implemented in malloc.c)
 */

/* External initialization functions - implemented in bio.c, tty.c, alloc.c */
extern void binit(void);
extern void cinit(void);
extern void iinit(void);

/*
 * Detect available memory
 * x86 specific - replaces PDP-11 memory sizing
 */
static void detect_memory(void) {
    /* For now, assume 16MB of memory
     * TODO: Get memory map from multiboot info or probe
     */
    maxmem = (16 * 1024 * 1024) / 64;  /* In 64-byte units like V6 */
    
    printf("mem = %d KB\n", (maxmem * 64) / 1024);
    
    /* Limit to MAXMEM if larger */
    if (maxmem > MAXMEM) {
        maxmem = MAXMEM;
    }
}

/*
 * setup_clock - Initialize x86 timer
 * Replaces V6's clock detection
 */
static void setup_clock(void) {
    printf("clock: initializing PIT timer...\n");
    
    /* Program PIT channel 0 for HZ interrupts per second */
    /* PIT frequency is 1193182 Hz */
    uint16_t divisor = 1193182 / HZ;
    
    outb(0x43, 0x36);               /* Channel 0, lobyte/hibyte, square wave */
    outb(0x40, divisor & 0xFF);     /* Low byte */
    outb(0x40, divisor >> 8);       /* High byte */
    
    printf("clock: timer running at %d Hz\n", HZ);
}

/*
 * main - Kernel initialization
 * Ported from original V6 ken/main.c
 *
 * Functions:
 *  - Clear and configure memory
 *  - Initialize system clock
 *  - Setup process 0 (swapper)
 *  - Initialize subsystems
 *  - Fork process 1 (init)
 *  - Enter scheduler loop
 */
/* Storage for Process 1 (init) */
static struct user p1_u;
static uint8_t p1_stack[4096];

extern int kbd_getc(void);

extern void readi(struct inode *ip);
extern struct inode *namei(int (*func)(), int flag);
extern void iput(struct inode *ip);
extern void prele(struct inode *ip);
extern void plock(struct inode *ip);



/* Global TSS */
struct tss_entry tss;
extern uint64_t gdt[];

/* Setup TSS and GDT entry */
static void setup_tss(void) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss);
    
    /* TSS Descriptor in GDT[5] */
    /* Low: Base[15:0] | Limit[15:0] */
    uint32_t low = (base << 16) | (limit & 0xFFFF);
    /* High: Base[31:24] | G=0,Avl=0,Lim[19:16]=0 | P=1,DPL=0,Type=9(TSS32) | Base[23:16] */
    uint32_t high = (base & 0xFF000000) |
                    0x00008900 |
                    ((base >> 16) & 0xFF);
                    
    gdt[5] = ((uint64_t)high << 32) | low;
    
    /* Initialize TSS */
    tss.ss0 = 0x10; /* Kernel Data Segment */
    tss.esp0 = (uint32_t)u.u_stack + sizeof(u.u_stack);
    
    /* Load Task Register */
    __asm__ __volatile__("ltr %w0" : : "q"(0x28));
}

static void proc0_main(void) __attribute__((noreturn));
extern void switch_stack_and_call(uint32_t new_esp, void (*fn)(void))
    __attribute__((noreturn));

static void proc0_main(void) {
    /*
     * Phase 5: Create init process and enter scheduler
     */
    printf("\nBootstraping /etc/init...\n");
    
    /* 
     * Manually setup process 1
     */
    /* 
     * Create init process using newproc
     */
    extern int exec(void);
    int np = newproc();
    if (np == 1) {
        /* Child (P1) resumes here - we're in kernel mode */
        /* CRITICAL: After fork, the global 'u' points to parent's u-area in kernel memory!
         * The child is now running in its own memory space. The child's u-area is located 
         * at address: p_addr * 64 bytes
         * 
         * The child can access its own u-area through:
         * 1. The parent's kernel `u` symbol (points to PARENT's u-area) - WRONG
         * 2. Directly via p_addr calculation - CORRECT
         *
         * The parent's code updated u.u_ar0 before fork, but we're now in the child.
         * The child needs to set up its own u.u_ar0 to point to the child's frame!
         */
        struct proc *cp = u.u_procp;  /* Get our own proc entry - this works because u_procp was updated by newproc */
        struct user *child_u_area = (struct user *)(cp->p_addr * 64);
        
        /* Just set up a minimal trap frame and call exec */
        static uint32_t init_frame[19];  /* Stable storage for init's trap frame */
        uint32_t *frame = init_frame;
        int i;
        
        /* Zero the frame */
        for (i = 0; i < 19; i++) {
            frame[i] = 0;
        }
        
        /* Set up user mode segments */
        frame[GS] = 0x23;
        frame[FS] = 0x23;
        frame[ES] = 0x23;
        frame[DS] = 0x23;
        frame[CS] = 0x1B;  /* User code segment */
        frame[USS] = 0x23; /* User stack segment */
        frame[EFLAGS] = 0x202; /* IF enabled */
        
        /* Point the CHILD's u_ar0 to this frame.
         * NOTE: We're setting u.u_ar0 which accesses the PARENT's u-area through the global symbol!
         * We actually need to set the CHILD's u_ar0, which is in the child's u-area memory!
         */
        child_u_area->u_ar0 = frame;
        
        /* But exec() accesses the global 'u', so we also need to set the global u's u_ar0
         * for exec() to work correctly. This is a hack but necessary given the architecture.
         */
        u.u_ar0 = frame;
        
        /* Set up exec arguments */
        u.u_arg[0] = (int)"/etc/init";
        u.u_arg[1] = 0;  /* No argv for now */
        
        /* Call exec - it will set up EIP and UESP in the frame via u.u_ar0 */
        exec();
        
        if (u.u_error) {
            printf("init: exec failed, error=%d\n", u.u_error);
            for(;;);
        }
        
        /* Now jump to user mode using the frame we set up */
        extern void return_to_user(uint32_t ip, uint32_t sp);
        return_to_user(u.u_ar0[EIP], u.u_ar0[UESP]);
        
        /* Should not return */
        panic("return_to_user failed in init");
    }

    printf("\n");
    printf("======================================\n");
    printf("Unix V6 x86 kernel initialization complete\n");
    printf("======================================\n\n");

    printf("Entering scheduler...\n");
    
    /* Enter scheduler loop (as Process 0) */
    /* Call swtch() directly - let P1 run */
    for (;;) {
        swtch();
    }
    
    /* Should never return */
    panic("scheduler returned");
}

void kmain(void) {
    int i;
    
    /* Initialize PIC */
    pic_init();

    /* Initialize serial port first for early debug output */
    serial_init();
    
    /* Clear VGA screen */
    vga_clear();
    
    /* Setup TSS for user mode interrupts */
    setup_tss();
    
    /* Print banner */
    printf("Unix V6 x86 Port\n");
    printf("================\n");
    printf("Ported from PDP-11 to x86 architecture\n\n");
    
    /*
     * Phase 1: Hardware initialization
     */
    printf("Initializing hardware...\n");
    
    /* Detect and configure memory */
    detect_memory();

    /* Initialize core allocator map */
    /* coremap manages memory from _end to maxmem */
    /* Calculate start in 64-byte clicks */
    /* _end is a byte address. We need to align up to 64 bytes */
    uint32_t mem_start_byte = (uint32_t)_end;
    uint32_t mem_start = (mem_start_byte + 63) / 64;
    
    /* maxmem is already in 64-byte clicks */
    if (mem_start >= maxmem) {
        panic("Kernel too large for memory");
    }
    
    int free_mem = maxmem - mem_start;
    
    /* Initialize empty map */
    /* Since coremap is bss (zeroed), it is already an empty map */
    
    /* Free the available memory into the map */
    mfree(coremap, free_mem, mem_start);
    
    printf("Memory map initialized: start=%x size=%d clicks (%d KB)\n", 
           mem_start, free_mem, (free_mem * 64) / 1024);
    
    /* Initialize clock */
    setup_clock();
    
    /*
     * Phase 2: Setup process 0 (swapper/scheduler)
     * From original V6 main.c
     */
    printf("\nSetting up process 0 (swapper)...\n");
    
    /* Clear process table */
    for (i = 0; i < NPROC; i++) {
        proc[i].p_stat = SNULL;
        proc[i].p_umask = 0;
        proc[i].p_sigmask = 0;
        proc[i].p_exit = 0;
        proc[i].p_pgrp = 0;
        proc[i].p_sid = 0;
    }
    
    /* Initialize process 0 */
    proc[0].p_stat = SRUN;
    proc[0].p_flag = SLOAD | SSYS;
    proc[0].p_pri = 127; /* Keep swapper lowest priority */
    /* proc[0].p_addr must be in 64-byte clicks */
    /* We allocate a dedicated backing store for Process 0 (swapper) */
    /* This ensures P0 has a safe place to save its state when switching */
    static struct user proc0_u __attribute__((aligned(64)));
    proc[0].p_addr = ((uint32_t)&proc0_u) / 64;
    proc[0].p_size = USIZE;
    proc[0].p_pid = 0;
    proc[0].p_ppid = 0;
    proc[0].p_umask = 0;
    proc[0].p_sigmask = 0;
    proc[0].p_exit = 0;
    proc[0].p_pgrp = proc[0].p_pid;
    proc[0].p_sid = proc[0].p_pid;
    curproc = &proc[0];
    curpri = proc[0].p_pri;
    
    /* Link user structure to process */
    u.u_procp = &proc[0];
    
    printf("Process 0 initialized: addr=%x size=%d\n", 
           proc[0].p_addr, proc[0].p_size);
    
    /*
     * Phase 3: Initialize kernel subsystems
     */
    printf("\nInitializing kernel subsystems...\n");
    
    /* Initialize ramdisk FIRST - before binit */
    extern void rd_init(void);
    extern void ide_init(void);
    rd_init();
    ide_init();

    /* Initialize buffer cache */
    binit();
    
    /* Initialize character device structures */
    cinit();
    
    /* Register console driver (Major 0) */
    extern struct cdevsw con_cdevsw;
    cdevsw[0] = con_cdevsw;
    nchrdev = 1;

    /* Initialize inode and file system */
    iinit();
    
    /*
     * Phase 4: Get root directory
     */
    printf("\nRoot filesystem initialization...\n");

    /* Mount root */
    rootdev = 0;  /* Ramdisk is major 0 */
    
    /* Get root inode - this reference is kept permanently by the kernel */
    rootdir = iget(rootdev, 1);
    if (rootdir == NULL) {
        panic("cannot mount root");
    }
    prele(rootdir);  /* Unlock but keep reference (i_count stays 1) */
    
    /* Get a SEPARATE reference for process 0's current directory */
    /* This is the reference that will be iput'd when process exits */
    u.u_cdir = iget(rootdev, 1);
    if (u.u_cdir == NULL) {
        panic("cannot get root for u.u_cdir");
    }
    prele(u.u_cdir);  /* Unlock but keep reference */
    
    printf("Root mounted. rootdir=%x u.u_cdir=%x i_count=%d\n", 
           rootdir, u.u_cdir, rootdir->i_count);

    /* 
     * Create /dev/console on the fly for init process
     * We allocate a transient inode for major 0, minor 0
     */
    extern struct inode *ialloc(dev_t dev);
    struct inode *cp = ialloc(rootdev);
    if (cp) {
        cp->i_mode = IFCHR | 0600;
        cp->i_addr[0] = 0; /* Major 0 (Console), Minor 0 */
        cp->i_nlink = 1;
        cp->i_flag |= IUPD | IACC;

        /* Assign to file descriptor 0, 1, 2 */
        /* Note: This is hacky direct assignment for bootstrap */
        struct file *fp = falloc();
        if (fp) {
            fp->f_flag = FREAD | FWRITE;
            fp->f_inode = cp;
            fp->f_inode->i_count++;
            prele(cp);
            
            u.u_ofile[0] = fp;
            u.u_ofile[1] = fp;
            u.u_ofile[2] = fp;
            
            /* falloc increments count, so we have 1 reference */
            /* We assigned it 3 times, let's bump count */
            fp->f_count = 3; 
            
            printf("Console opened on fd 0,1,2\n");
        } else {
            iput(cp);
        }
    }
    
    /* Switch to process 0's u-area stack before creating init */
    uint32_t new_esp = (uint32_t)u.u_stack + sizeof(u.u_stack);
    new_esp &= ~3; /* Align to 4 bytes */
    switch_stack_and_call(new_esp, proc0_main);
    __builtin_unreachable();
}

/*
 * estabur - Establish User Registers (Memory Management)
 * 
 * Validates the size of text, data, and stack segments.
 * Sets up the process prototypes for memory mapping.
 * 
 * nt: text size in 64-byte clicks
 * nd: data size
 * ns: stack size
 * sep: separation flag (0=combined I/D, 1=separate)
 * 
 * Returns 0 on success, -1 on failure.
 */
int estabur(int nt, int nd, int ns, int sep) {
    /* Check limits */
    /* x86 has much larger address space, but we check against physical RAM size */
    
    if (sep) {
        /* We typically don't support separate I/D in this simple flat port 
         * unless we use sophisticated segmentation.
         * For now, treat as error or ignore? 
         * Classic V6 treated sep=1 as valid if hardware supported it.
         * We will just check total size.
         */
    }
    
    if (nt + nd + ns + USIZE > maxmem) {
        u.u_error = ENOMEM;
        return -1;
    }
    
    /* 
     * In a flat memory model (all segments 0-4GB), we don't need to change 
     * segmentation registers here. 
     * Real protection would require updating GDT/LDT or CR3 (Paging).
     * For now, we assume a cooperative or single-user-like environment 
     * or rely on the fact effectively we are valid.
     */
     
    return 0;
}
