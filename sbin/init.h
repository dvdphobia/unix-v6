/*
 * init.c - System initialization (sbin/init)
 *
 * Unix V6 x86 Port
 * The init process is the first user process (PID 1).
 * It initializes the system and spawns the shell.
 *
 * In the original V6, init would:
 * 1. Read /etc/ttys to configure terminals
 * 2. Fork getty processes for each terminal
 * 3. Handle orphaned processes
 *
 * In this port, init runs a built-in shell directly
 * since we don't have real exec() yet.
 */

#ifndef _SBIN_INIT_H_
#define _SBIN_INIT_H_

/*
 * Start the init process
 * Called from kernel after bootstrap
 */
void start_init(void);

/*
 * Init configuration
 */
#define INIT_SHELL      "/bin/sh"       /* Default shell */
#define INIT_CONSOLE    "/dev/console"  /* Console device */
#define INIT_TTYS       "/etc/ttys"     /* Terminal config */


#endif /* _SBIN_INIT_H_ */
