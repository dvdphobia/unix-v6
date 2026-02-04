/* crtend.c - End markers for constructor/destructor lists */

typedef void (*func_ptr)(void);

/* End of constructor list */
static func_ptr __CTOR_END__[1] 
    __attribute__((used, section(".ctors"), aligned(sizeof(func_ptr)))) 
    = { (func_ptr) 0 };

/* End of destructor list */
static func_ptr __DTOR_END__[1] 
    __attribute__((used, section(".dtors"), aligned(sizeof(func_ptr)))) 
    = { (func_ptr) 0 };
