#include "../syscalls.h"
#include "../libc/fcntl.h"
#include "../libc/unistd.h"

/* Simple strlen */
static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

/* Write string to stdout */
static void put_str(const char *s) {
    write(1, s, str_len(s));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    put_str("TinyCC: Tiny C Compiler - version 0.9.27\n");
    put_str("\n");
    put_str("Note: TinyCC is not yet fully ported to Unix V6.\n");
    put_str("      This is a placeholder implementation.\n");
    put_str("\n");
    put_str("Usage: tcc [options] infile(s)\n");
    put_str("\n");
    put_str("General options:\n");
    put_str("  -v             show version\n");
    put_str("  -c             compile only\n");
    put_str("  -o <file>      set output filename\n");
    put_str("  -run <args>    run compiled source\n");
    put_str("\n");
    put_str("For full TinyCC functionality, please use the host system compiler.\n");
    
    return 1;
}
