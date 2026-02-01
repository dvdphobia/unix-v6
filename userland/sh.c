#include "syscalls.h"

#define BUF_SIZE 128
#define MAX_ARGS 8

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int tokenize(char *buf, char **argv, int max_args) {
    int argc = 0;
    char *p = buf;

    while (*p && argc < max_args - 1) {
        while (*p && is_space(*p)) {
            *p++ = '\0';
        }
        if (!*p) break;
        argv[argc++] = p;
        while (*p && !is_space(*p)) p++;
    }
    argv[argc] = 0;
    return argc;
}

int main(int argc, char **argv) {
    char buf[BUF_SIZE];
    char *args[MAX_ARGS];
    int pid;

    (void)argc;
    (void)argv;

    write(1, "$ ", 2);
    while (1) {
        int n = read(0, buf, sizeof(buf) - 1);
        if (n <= 0) {
            write(1, "\n", 1);
            exit(0);
        }
        buf[n] = '\0';

        argc = tokenize(buf, args, MAX_ARGS);
        if (argc == 0) {
            write(1, "$ ", 2);
            continue;
        }

        if (args[0][0] == 'c' && args[0][1] == 'd' && args[0][2] == '\0') {
            if (argc < 2) {
                write(1, "cd: missing operand\n", 21);
            } else if (chdir(args[1]) < 0) {
                write(1, "cd: failed\n", 11);
            }
            write(1, "$ ", 2);
            continue;
        }

        if (args[0][0] == 'e' && args[0][1] == 'x' && args[0][2] == 'i' && args[0][3] == 't' && args[0][4] == '\0') {
            exit(0);
        }

        if (args[0][0] == 'h' && args[0][1] == 'e' && args[0][2] == 'l' && args[0][3] == 'p' && args[0][4] == '\0') {
            write(1, "Available commands:\n", 20);
            write(1, "  cd <dir>   - Change directory\n", 32);
            write(1, "  ls         - List files\n", 26);
            write(1, "  cat <file> - Display file content\n", 36);
            write(1, "  echo <msg> - Print message\n", 29);
            write(1, "  pwd        - Print working directory\n", 39);
            write(1, "  exit       - Exit shell\n", 26);
            write(1, "  help       - Show this message\n", 33);
            write(1, "$ ", 2);
            continue;
        }

        pid = fork();
        if (pid == 0) {
            char path[64];
            if (args[0][0] == '/') {
                exec(args[0], args);
            } else {
                int i = 0;
                const char *prefix = "/bin/";
                while (prefix[i]) {
                    path[i] = prefix[i];
                    i++;
                }
                int j = 0;
                while (args[0][j] && i < (int)sizeof(path) - 1) {
                    path[i++] = args[0][j++];
                }
                path[i] = '\0';
                exec(path, args);
            }
            write(1, "command not found\n", 18);
            exit(1);
        }

        wait(0);
        write(1, "$ ", 2);
    }
    return 0;
}
