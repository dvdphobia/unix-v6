/* help.c - Display available commands */

#include "../syscalls.h"
#include "../libc/fcntl.h"
#include "../libc/unistd.h"

int strlen(const char *s) {
    int n;
    for (n = 0; *s; s++)
        n++;
    return n;
}

int main(void) {
    const char *help_text = 
        "Available commands:\n"
        "  cat <file>     - Display file contents\n"
        "  clear          - Clear the screen\n"
        "  echo <text>    - Print text to screen\n"
        "  help           - Show this help message\n"
        "  hello          - Test program\n"
        "  ls [dir]       - List directory contents\n"
        "  ps             - Show process list\n"
        "  pwd            - Print working directory\n"
        "  uname          - Show system information\n"
        "  netdemo        - Network loop test\n"
        "\nBuilt-in shell commands:\n"
        "  cd <dir>       - Change directory\n"
        "  exit           - Exit shell\n";
    
    write(1, help_text, strlen(help_text));
    return 0;
}
