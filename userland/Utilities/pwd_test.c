/* pwd_test.c - Test getcwd syscall */

#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    char buf[256];
    char *result;
    
    result = getcwd(buf, sizeof(buf));
    
    if (result == NULL) {
        write(1, "pwd: getcwd failed\n", 19);
        return 1;
    }
    
    write(1, result, 256);  /* Simple - prints up to 256 chars */
    write(1, "\n", 1);
    return 0;
}
