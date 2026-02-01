.global _start
_start:
    movl (%esp), %eax      # argc
    lea 4(%esp), %ebx      # argv
    push %ebx
    push %eax
    call main
    addl $8, %esp
    movl %eax, %ebx
    movl $1, %eax
    int $0x80
