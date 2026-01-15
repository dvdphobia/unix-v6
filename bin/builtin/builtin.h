/*
 * builtin.h - Shell Built-in Commands
 *
 * Unix V6 x86 Port
 * These are kernel-internal shell commands that run
 * as part of the init process shell.
 */

#ifndef _BUILTIN_H_
#define _BUILTIN_H_

/*
 * Built-in command function declarations
 */

/* File system navigation */
void builtin_ls(const char *path);      /* List directory contents */
void builtin_cd(const char *path);      /* Change directory */
void builtin_pwd(void);                  /* Print working directory */
void builtin_cat(const char *path);      /* Display file contents */
void builtin_mkdir(const char *path);    /* Create directory */

/* System information */
void builtin_ps(void);                   /* List processes */
void builtin_uname(void);                /* System information */
void builtin_date(void);                 /* Show date/time */
void builtin_uptime(void);               /* System uptime */

/* Shell utilities */
void builtin_echo(const char *args);     /* Echo arguments */
void builtin_clear(void);                /* Clear screen */
void builtin_help(void);                 /* Show help */

/* System control */
void builtin_reboot(void);               /* Reboot system */
void builtin_halt(void);                 /* Halt system */

#endif /* _BUILTIN_H_ */
