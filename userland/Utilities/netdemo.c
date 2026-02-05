#include <fcntl.h>
#include <unistd.h>

int main(void) {
    write(1, "netdemo: network not implemented\n", 35);
    return 0;
}
