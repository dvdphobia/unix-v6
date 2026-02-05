#include <fcntl.h>
#include <unistd.h>

int main(void) {
    write(1, "hello from userland\n", 21);
    return 0;
}
