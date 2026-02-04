/* crt1.c - Alternative minimal C startup */

/*
 * Minimal CRT startup for systems that don't need
 * constructor/destructor support
 */

extern int main(int argc, char **argv, char **envp);
extern void exit(int status) __attribute__((noreturn));

char **environ = 0;

#if defined(__i386__) || defined(__i686__)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "xorl %%ebp, %%ebp\n"
        "popl %%esi\n"                  /* argc */
        "movl %%esp, %%edi\n"           /* argv */
        "leal 4(%%edi,%%esi,4), %%edx\n" /* envp */
        "andl $-16, %%esp\n"            /* align stack */
        "pushl %%edx\n"                 /* envp */
        "pushl %%edi\n"                 /* argv */
        "pushl %%esi\n"                 /* argc */
        "call main\n"
        "movl %%eax, %%ebx\n"
        "movl $1, %%eax\n"
        "int $0x80\n"
        "hlt\n"
        ::: "memory"
    );
}

#else

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "movl %%esp, %%eax\n"
        "andl $-16, %%esp\n"
        "pushl %%eax\n"
        "call _start_c\n"
        "hlt\n"
        ::: "eax", "memory"
    );
}

void __attribute__((used, noreturn))
_start_c(void *stack_ptr)
{
    long *sp = (long *)stack_ptr;
    int argc = (int)*sp++;
    char **argv = (char **)sp;
    char **envp = argv + argc + 1;
    
    environ = envp;
    
    int ret = main(argc, argv, envp);
    exit(ret);
    
    while (1);
}

#endif
