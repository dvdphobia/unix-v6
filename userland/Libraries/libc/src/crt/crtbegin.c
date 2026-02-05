/* crtbegin.c - Constructor/Destructor list management */

typedef void (*func_ptr)(void);

/* Start of constructor list */
static func_ptr __CTOR_LIST__[1] 
    __attribute__((used, section(".ctors"), aligned(sizeof(func_ptr)))) 
    = { (func_ptr) -1 };

/* Start of destructor list */
static func_ptr __DTOR_LIST__[1] 
    __attribute__((used, section(".dtors"), aligned(sizeof(func_ptr)))) 
    = { (func_ptr) -1 };

/* Call all global constructors */
void __attribute__((section(".init"), used))
__do_global_ctors_aux(void)
{
    func_ptr *p = __CTOR_LIST__ + 1;
    
    /* Skip the -1 marker and call each constructor */
    while (*p != (func_ptr) 0) {
        (*p)();
        p++;
    }
}

/* Call all global destructors */
void __attribute__((section(".fini"), used))
__do_global_dtors_aux(void)
{
    static int completed = 0;
    
    if (completed)
        return;
    
    func_ptr *p = __DTOR_LIST__ + 1;
    
    /* Call each destructor */
    while (*p != (func_ptr) 0) {
        (*p)();
        p++;
    }
    
    completed = 1;
}

/* These are called automatically by _init and _fini */
void __attribute__((constructor(65535)))
_crtbegin_init(void)
{
    __do_global_ctors_aux();
}

void __attribute__((destructor(101)))
_crtbegin_fini(void)
{
    __do_global_dtors_aux();
}
