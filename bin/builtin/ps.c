/*
 * ps.c - Process status
 *
 * Unix V6 x86 Port
 * Built-in shell command to display process information.
 */

#include "../../kernel/include/types.h"
#include "../../kernel/include/param.h"
#include "../../kernel/include/proc.h"

/* External dependencies */
extern void printf(const char *fmt, ...);
extern struct proc proc[];

/* Process state names */
static const char *stat_name(int stat) {
    switch (stat) {
        case SSLEEP:  return "SLEEP";
        case SWAIT:   return "WAIT";
        case SRUN:    return "RUN";
        case SIDL:    return "IDLE";
        case SZOMB:   return "ZOMBIE";
        case SSTOP:   return "STOP";
        default:      return "?";
    }
}

/*
 * builtin_ps - Display process status
 */
void builtin_ps(void) {
    int i;
    int count = 0;
    
    printf("PID  PPID  STAT    NAME\n");
    printf("---  ----  ------  --------\n");
    
    for (i = 0; i < NPROC; i++) {
        if (proc[i].p_stat != SNULL) {
            const char *name;
            if (proc[i].p_pid == 0) {
                name = "swapper";
            } else if (proc[i].p_pid == 1) {
                name = "init";
            } else {
                name = "process";
            }
            printf("%d    %d     %s   [%s]\n", 
                   proc[i].p_pid, 
                   proc[i].p_ppid,
                   stat_name(proc[i].p_stat),
                   name);
            count++;
        }
    }
    
    printf("\nTotal: %d processes\n", count);
}
