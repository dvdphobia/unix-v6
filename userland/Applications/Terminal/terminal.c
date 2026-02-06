/*
 * terminal.c - Interactive terminal application for Unix V6
 * A simple terminal emulator with line editing and command history
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* V6 headers for wait macros in case stdlib doesn't have them */
#include <sys/wait.h>

#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif

#define BUFFER_SIZE 512
#define HISTORY_SIZE 50
#define MAX_ARGS 32

typedef struct {
    char commands[HISTORY_SIZE][BUFFER_SIZE];
    int count;
    int current;
} History;

/* Fixed initialization warning */
static History history = { .count = 0, .current = 0 };

void add_to_history(const char *cmd) {
    if (history.count < HISTORY_SIZE) {
        strcpy(history.commands[history.count], cmd);
        history.count++;
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strcpy(history.commands[i], history.commands[i + 1]);
        }
        strcpy(history.commands[HISTORY_SIZE - 1], cmd);
    }
    history.current = history.count;
}

void print_prompt(void) {
    char cwd[256] = "/";
    getcwd(cwd, sizeof(cwd));
    printf("unix-v6:%s$ ", cwd);
    fflush(stdout);
}

void print_welcome(void) {
    printf("\n");
    printf("=========================================\n");
    printf("   Unix V6 Terminal - Interactive Shell\n");
    printf("   Type 'help' for available commands\n");
    printf("=========================================\n");
    printf("\n");
}

void print_help(void) {
    printf("\nBuilt-in Commands:\n");
    printf("  help        - Show this help message\n");
    printf("  history     - Show command history\n");
    printf("  clear       - Clear the screen\n");
    printf("  pwd         - Print working directory\n");
    printf("  cd <dir>    - Change directory\n");
    printf("  exit        - Exit terminal\n");
    printf("  quit        - Exit terminal\n");
    printf("\nYou can also run any system program available in the OS.\n\n");
}

void print_history(void) {
    printf("\nCommand History:\n");
    for (int i = 0; i < history.count; i++) {
        printf("  %2d: %s\n", i + 1, history.commands[i]);
    }
    printf("\n");
}

int parse_command(char *line, char **args) {
    int argc = 0;
    int in_word = 0;
    int i = 0;
    
    while (line[i] != '\0' && argc < MAX_ARGS - 1) {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') {
            if (in_word) {
                line[i] = '\0';
                in_word = 0;
            }
        } else {
            if (!in_word) {
                args[argc++] = &line[i];
                in_word = 1;
            }
        }
        i++;
    }
    args[argc] = NULL;
    
    return argc;
}

int execute_builtin(const char *cmd, char **args) {
    (void)args;  /* Unused in most cases */
    
    if (strcmp(cmd, "help") == 0) {
        print_help();
        return 1;
    } else if (strcmp(cmd, "history") == 0) {
        print_history();
        return 1;
    } else if (strcmp(cmd, "clear") == 0) {
        /* Escape sequence to clear screen (ANSI) */
        printf("\033[2J\033[H");
        return 1;
    } else if (strcmp(cmd, "pwd") == 0) {
        char buf[256];
        if (getcwd(buf, sizeof(buf))) {
            printf("%s\n", buf);
        } else {
            printf("pwd: error getting cwd\n");
        }
        return 1;
    } else if (strcmp(cmd, "cd") == 0) {
        if (args[1] != NULL) {
            if (chdir(args[1]) != 0) {
                printf("cd: cannot access '%s'\n", args[1]);
            }
        } else {
            chdir("/");
        }
        return 1;
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        printf("\nGoodbye!\n");
        exit(0);
    }
    
    return 0;
}

void execute_command(char *line) {
    if (strlen(line) == 0) {
        return;
    }
    
    /* Remove trailing newline */
    int len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    char *args[MAX_ARGS];
    char cmd_copy[BUFFER_SIZE];
    strcpy(cmd_copy, line);
    
    int argc = parse_command(cmd_copy, args);
    if (argc == 0) {
        return;
    }
    
    if (execute_builtin(args[0], args)) {
        return;
    }
    
    /* Try to find command in PATH or /bin/ */
    char full_path[BUFFER_SIZE];
    
    /* First check if it's an absolute path */
    if (args[0][0] == '/') {
        strcpy(full_path, args[0]);
    } else {
        /* Try /bin/ directory */
        strcpy(full_path, "/bin/");
        strcat(full_path, args[0]);
    }
    
    int pid = fork();
    if (pid == 0) {
        /* Child process */
        /* Set pgid for job control if we had it */
        /* setpgid(0, 0); */
        
        execv(full_path, args);
        
        /* If execv returns, it failed */
        /* Try without /bin prefix if it failed */
        if (args[0][0] != '/') {
             execv(args[0], args);
        }
        
        printf("%s: command not found\n", args[0]);
        exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        printf("fork failed\n");
    }
}

int main(int argc, char *argv[]) {
    (void)argc;  /* Unused */
    (void)argv;  /* Unused */
    char input[BUFFER_SIZE];
    
    print_welcome();
    
    while (1) {
        print_prompt();
        
        /* memset input to avoid garbage */
        memset(input, 0, sizeof(input));
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        /* Handle empty lines */
        if (input[0] == '\n' || input[0] == '\0') continue;
        
        if (strlen(input) > 0) {
            /* Strip newline from history */
            char *nl = strchr(input, '\n');
            if (nl) *nl = 0;
            add_to_history(input);
            execute_command(input);
        }
    }
    
    printf("\nTerminal closed.\n");
    return 0;
}
