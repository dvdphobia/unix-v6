#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG     1
#define WUNTRACED   2

#define WIFEXITED(s)    (((s) & 0xFF) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xFF)
#define WIFSIGNALED(s)  (((s) & 0xFF) != 0 && ((s) & 0xFF) != 0x7F)
#define WTERMSIG(s)     ((s) & 0x7F)
#define WIFSTOPPED(s)   (((s) & 0xFF) == 0x7F)
#define WSTOPSIG(s)     (((s) >> 8) & 0xFF)

pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

#endif /* _SYS_WAIT_H */
