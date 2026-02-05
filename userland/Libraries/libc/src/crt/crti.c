/* crti.c - Initialization functions (C implementation) */

/*
 * This provides the _init and _fini function prologues in C.
 * GCC can insert constructor/destructor calls between crti and crtn.
 */

void __attribute__((section(".init")))
_init(void)
{
    /* Compiler will insert constructor calls here */
    /* This is the prologue - crtbegin adds the body */
}

void __attribute__((section(".fini")))
_fini(void)
{
    /* Compiler will insert destructor calls here */
    /* This is the prologue - crtend adds the body */
}
