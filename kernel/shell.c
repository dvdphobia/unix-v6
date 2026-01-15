/* shell.c - Simple Kernel Shell (Init Process)
 *
 * Unix V6 x86 Port
 * Implements a basic shell running as Process 1 (init).
 *
 * This is a kernel-mode shell with built-in commands.
 * In a full implementation, this would exec /bin/sh.
 */

#include "include/types.h"
#include "include/param.h"
#include "include/user.h"
#include "include/inode.h"
#include "include/proc.h"

#define CMD_BUF_SIZE 128

/* External Dependencies - Kernel */
extern struct user u;
extern struct proc proc[NPROC];
extern struct inode *rootdir;
extern void printf(const char *fmt, ...);
extern void panic(const char *msg);
extern void vga_putchar(char c);
extern void serial_putchar(char c);
extern void vga_clear(void);
extern int kbd_getc(void);
extern void idle(void);

/* External Dependencies - Built-in commands from /bin/builtin */
extern void builtin_ls(const char *path);
extern void builtin_cd(const char *path);
extern void builtin_pwd(void);
extern void builtin_echo(const char *args);
extern void builtin_ps(void);
extern void builtin_uname(void);
extern void builtin_cat(const char *path);

/* String comparison */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* String length */
static int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

/*
 * builtin_help - Display available commands
 */
static void builtin_help(void) {
    printf("\nUnix V6 x86 Shell - Built-in Commands\n");
    printf("======================================\n\n");
    printf("  File System:\n");
    printf("    ls [path]   - List directory contents\n");
    printf("    cd <dir>    - Change directory\n");
    printf("    pwd         - Print working directory\n");
    printf("    cat <file>  - Display file contents\n");
    printf("\n");
    printf("  System:\n");
    printf("    ps          - List processes\n");
    printf("    uname       - System information\n");
    printf("    clear       - Clear screen\n");
    printf("\n");
    printf("  Other:\n");
    printf("    echo <text> - Echo text\n");
    printf("    help        - This help message\n");
    printf("    panic       - Trigger kernel panic (debug)\n");
    printf("\n");
}

/*
 * handle_command - Process a command line
 */
static void handle_command(char *cmd) {
    char *args;
    
    /* Skip empty lines */
    if (cmd[0] == '\0') return;
    
    /* Split command and arguments */
    args = cmd;
    while (*args && *args != ' ') args++;
    if (*args == ' ') {
        *args = '\0';
        args++;
        /* Skip leading spaces in args */
        while (*args == ' ') args++;
        if (*args == '\0') args = NULL;
    } else {
        args = NULL;
    }
    
    /* Built-in commands */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        builtin_help();
    }
    else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(cmd, "panic") == 0) {
        panic("User requested panic");
    }
    else if (strcmp(cmd, "ls") == 0) {
        builtin_ls(args);
    }
    else if (strcmp(cmd, "cd") == 0) {
        builtin_cd(args);
    }
    else if (strcmp(cmd, "pwd") == 0) {
        builtin_pwd();
    }
    else if (strcmp(cmd, "ps") == 0) {
        builtin_ps();
    }
    else if (strcmp(cmd, "echo") == 0) {
        builtin_echo(args);
    }
    else if (strcmp(cmd, "uname") == 0) {
        builtin_uname();
    }
    else if (strcmp(cmd, "cat") == 0) {
        builtin_cat(args);
    }
    else {
        printf("%s: command not found\n", cmd);
        printf("Type 'help' for available commands.\n");
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
