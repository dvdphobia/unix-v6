/*
 * echo.c - Echo arguments
 *
 * Unix V6 x86 Port
 * Built-in shell command to echo text to console.
 */

extern void printf(const char *fmt, ...);

/*
 * builtin_echo - Echo arguments to stdout
 */
void builtin_echo(const char *args) {
    if (args && args[0] != '\0') {
        printf("%s\n", args);
    } else {
        printf("\n");
    }
}
