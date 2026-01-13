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

/*
 * Icode is the bootstrap program executed in user mode
 * to bring up the system.
 * Execs /etc/init
 */
static uint8_t icode[] = {
    0xb8, 0x0b, 0x00, 0x00, 0x00,  /* mov eax, 11 (exec) */
    0xb9, 0x14, 0x00, 0x00, 0x00,  /* mov ecx, 20 (addr of path) */
    0xba, 0x20, 0x00, 0x00, 0x00,  /* mov edx, 32 (addr of argv) */
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
struct user u;

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

static void vga_clear(void) {
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

#define CMD_BUF_SIZE 128

/*
 * strcmp - String comparison helper
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/*
 * handle_command - Process a command line
 */
void handle_command(char *cmd) {
    if (cmd[0] == 0) return; /* Empty line */
    
    if (strcmp(cmd, "help") == 0) {
        printf("Available commands:\n");
        printf("  help   - Show this help\n");
        printf("  clear  - Clear screen\n");
        printf("  ls     - List files (stub)\n");
        printf("  ps     - List processes\n");
        printf("  panic  - Test kernel panic\n");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
        printf("Unix V6 x86 Port\n");
        return;
    }
    else if (strcmp(cmd, "panic") == 0) {
        panic("User requested panic");
    }
    else if (strcmp(cmd, "ls") == 0) {
       printf("bin  etc  dev  usr  tmp  (simulated)\n");
    }
    else if (strcmp(cmd, "ps") == 0) {
        printf("PID  PPID STAT ADDR\n");
        for(int i=0; i<NPROC; i++) {
            if (proc[i].p_stat != SNULL) {
                printf("%d    %d    %d    %x", proc[i].p_pid, proc[i].p_ppid, proc[i].p_stat, proc[i].p_addr);
                if (i == 0) printf(" (swapper)\n");
                else if (i == 1) printf(" (init)\n");
                else printf("\n");
            }
        }
    }
    else {
        printf("Unknown command: %s\n", cmd);
    }
}

/*
 * start_init - Entry point for Process 1
 */
void start_init(void) {
    char buf[CMD_BUF_SIZE];
    int pos = 0;

    /* We are now running as Process 1! */
    printf("\n[P1] Process 1 started!\n");
    printf("[P1] Simulating exec of /etc/init...\n");
    
    printf("\nWelcome to Unix V6 x86 Port!\n");
    printf("Type 'help' for commands.\n");
    printf("# ");
    
    for (;;) {
       int c = kbd_getc();
       if (c != -1) {
           /* Handle enter/newline */
           if (c == '\n' || c == '\r') {
               buf[pos] = 0; /* Null terminate */
               printf("\n");
               handle_command(buf);
               if (pos > 0 || buf[0] == 0) printf("# "); /* Prompt */
               pos = 0;
           }
           /* Handle backspace */
           else if (c == '\b' || c == 0x7F) {
               if (pos > 0) {
                   pos--;
                   /* Erase char on screen (backspace, space, backspace) */
                   vga_putchar('\b'); 
                   vga_putchar(' ');
                   vga_putchar('\b');
                   /* Serial backspace */
                   serial_putchar('\b');
                   serial_putchar(' ');
                   serial_putchar('\b');
               }
           }
           /* Normal character */
           else {
               if (pos < CMD_BUF_SIZE - 1) {
                   buf[pos++] = c;
                   vga_putchar(c);
                   serial_putchar(c);
               }
           }
       } else {
           idle();
       }
    }
}

void kmain(void) {
    int i;
    
    /* Initialize PIC */
    pic_init();

    /* Initialize serial port first for early debug output */
    serial_init();
    
    /* Clear VGA screen */
    vga_clear();
    
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
    }
    
    /* Initialize process 0 */
    proc[0].p_stat = SRUN;
    proc[0].p_flag = SLOAD | SSYS;
    proc[0].p_addr = (uint32_t)&u;      /* User structure address */
    proc[0].p_size = USIZE;
    proc[0].p_pid = 0;
    proc[0].p_ppid = 0;
    curproc = &proc[0];
    
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
    rd_init();

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
    rootdir = iget(rootdev, 1);
    iput(rootdir); 
    u.u_cdir = iget(rootdev, 1);
    rootdir = u.u_cdir;
    
    printf("Root mounted. rootdir=%x\n", rootdir);

    /* 
     * Manually open /dev/console for init process
     * We do this for Process 0, which Process 1 inherits
     */
    struct inode *cp;
    cp = iget(rootdev, 5); /* /dev/console is inode 5 */
    if (cp) {
        /* Assign to file descriptor 0, 1, 2 */
        /* Note: This is hacky direct assignment for bootstrap */
        struct file *fp = falloc();
        if (fp) {
            fp->f_flag = FREAD | FWRITE;
            fp->f_inode = cp;
            fp->f_inode->i_count++;
            
            u.u_ofile[0] = fp;
            u.u_ofile[1] = fp;
            u.u_ofile[2] = fp;
            
            /* falloc increments count, so we have 1 reference */
            /* We assigned it 3 times, let's bump count */
            fp->f_count = 3; 
            
            printf("Console opened on fd 0,1,2\n");
        }
    }
    
    /*
     * Phase 5: Create init process and enter scheduler
     */
    printf("\nBootstraping /etc/init...\n");
    
    /* 
     * Manually setup process 1
     */
    struct proc *p1 = &proc[1];
    p1->p_stat = SRUN;
    p1->p_flag = SLOAD;
    p1->p_pid = 1;
    p1->p_ppid = 0;
    p1->p_size = USIZE;
    p1->p_uid = 0;
    p1->p_textp = NULL;
    
    /* Allocate storage for P1 */
    p1->p_addr = (uint32_t)&p1_u;
    
    /* Setup P1 context */
    /* 1. Copy current U (P0) as a template */
    bcopy(&u, &p1_u, sizeof(struct user));
    
    /* 2. Setup P1's stack */
    /* Push start_init address onto P1's stack */
    uint32_t *sp = (uint32_t *)&p1_stack[4096];
    *(--sp) = (uint32_t)start_init;
    
    /* 3. Setup P1's saved context to resume at sp */
    p1_u.u_rsav[0] = (uint32_t)sp;      /* ESP points to return address */
    p1_u.u_rsav[1] = (uint32_t)sp;      /* EBP (can be same as ESP for now) */
    
    printf("\n");
    printf("======================================\n");
    printf("Unix V6 x86 kernel initialization complete\n");
    printf("======================================\n\n");
    
    printf("Entering scheduler...\n");
    
    /* Enter scheduler loop (as Process 0) */
    sched();
    
    /* Should never return */
    panic("sched returned");
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
    /* x86 has much larger address space, but we keep V6 limits concept? 
       Actually V6 limits were 64KB. We probably allow more.
       Let's check against MAXMEM and ensure no overflow. */
       
    if (sep) {
        /* Check separate limits if relevant */
    }
    
    if (nt + nd + ns + USIZE > MAXMEM) {
        u.u_error = ENOMEM;
        return -1;
    }
    
    /* 
     * Setup hardware registers/page tables.
     * For x86 simple port, we might assume flat model or setup GDT/LDT 
     * but we don't have fields in user structure yet for this specific port style.
     * We assume this is handled by core memory management.
     */
     
    return 0;
}
