#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char *args[] = {"/bin/winserver", 0};
    execv(args[0], args);
    printf("Failed to start WindowServer\n");
    return 1;
}
