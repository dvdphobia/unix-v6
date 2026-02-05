/* crt0.c - Portable C Runtime Startup (C implementation) */

/*
 * This is a portable C implementation of crt0 that can be compiled
 * for different architectures. Architecture-specific assembly is
 * kept to an absolute minimum.
 */

/* External symbols */
extern int main(int argc, char **argv, char **envp);
extern void exit(int status) __attribute__((noreturn));
extern void _init(void);
extern void _fini(void);

/* Global environment pointer */
char **environ = 0;

/*
 * Architecture-specific startup code
 * This small assembly stub is needed to get initial stack state
 */
#if defined(__i386__) || defined(__i686__)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "xorl %%ebp, %%ebp\n"           /* Clear frame pointer */
        "movl %%esp, %%eax\n"            /* Save stack pointer */
        "andl $-16, %%esp\n"             /* Align stack to 16 bytes */
        "pushl %%eax\n"                  /* Push original stack pointer */
        "call _start_c\n"                /* Call C startup */
        "hlt\n"                          /* Should never reach here */
        ::: "eax", "memory"
    );
}

#elif defined(__x86_64__) || defined(__amd64__)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "xorq %%rbp, %%rbp\n"           /* Clear frame pointer */
        "movq %%rsp, %%rdi\n"           /* Stack pointer as 1st arg */
        "andq $-16, %%rsp\n"            /* Align stack to 16 bytes */
        "call _start_c\n"               /* Call C startup */
        "hlt\n"
        ::: "rdi", "memory"
    );
}

#elif defined(__arm__) || defined(__aarch64__)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "mov r0, sp\n"                  /* Stack pointer as argument */
        "bl _start_c\n"                 /* Call C startup */
        "b .\n"                         /* Infinite loop */
        ::: "r0", "memory"
    );
}

#elif defined(__riscv)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "mv a0, sp\n"                   /* Stack pointer as argument */
        "call _start_c\n"               /* Call C startup */
        "j .\n"                         /* Infinite loop */
        ::: "a0", "memory"
    );
}

#else
#error "Unsupported architecture for CRT0"
#endif

/*
 * Portable C startup code
 * Parses argc/argv/envp and calls main()
 */
void __attribute__((used, noreturn))
_start_c(void *stack_ptr)
{
    /* Parse stack layout:
     * [argc]
     * [argv[0]]
     * [argv[1]]
     * ...
     * [argv[argc-1]]
     * [NULL]
     * [envp[0]]
     * [envp[1]]
     * ...
     * [NULL]
     */
    
    long *sp = (long *)stack_ptr;
    int argc = (int)*sp++;
    char **argv = (char **)sp;
    
    /* Find envp (after argv + NULL terminator) */
    char **envp = argv;
    int i;
    for (i = 0; i <= argc; i++) {
        envp++;
    }
    
    /* Set global environ */
    environ = envp;
    
    /* Call init functions (constructors) */
    _init();
    
    /* Call main program */
    int ret = main(argc, argv, envp);
    
    /* Call fini functions (destructors) */
    _fini();
    
    /* Exit with main's return value */
    exit(ret);
    
    /* Never reached */
    while (1);
}
