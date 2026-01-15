/*
 * uname.c - System information
 *
 * Unix V6 x86 Port
 * Built-in shell command to display system information.
 */

extern void printf(const char *fmt, ...);

#define UNIX_VERSION "6"
#define UNIX_RELEASE "1.0"
#define UNIX_MACHINE "i386"

/*
 * builtin_uname - Display system information
 *
 * Mimics macOS/BSD uname output style.
 */
void builtin_uname(void) {
    printf("Unix V%s x86 Port\n", UNIX_VERSION);
    printf("  System:   Unix\n");
    printf("  Release:  V%s\n", UNIX_VERSION);
    printf("  Version:  %s\n", UNIX_RELEASE);
    printf("  Machine:  %s\n", UNIX_MACHINE);
    printf("  Origin:   Bell Labs, 1975 (PDP-11)\n");
    printf("  Port:     x86 architecture\n");
}
