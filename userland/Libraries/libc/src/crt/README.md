# C Runtime (CRT) - Portable Implementation

Portable C implementation of the C runtime startup code that works across different architectures.

## Architecture Support

This CRT implementation supports:
- ✅ **x86 (i386/i686)** - 32-bit Intel/AMD
- ✅ **x86-64 (amd64)** - 64-bit Intel/AMD
- ✅ **ARM (32-bit)** - ARM processors
- ✅ **ARM64 (aarch64)** - 64-bit ARM
- ✅ **RISC-V** - RISC-V processors
- ➕ **Easy to add more** - Just add new architecture case in crt0.c

## Files

### Core CRT Files

**crt0.c** - Full-featured startup
- Multi-architecture support with minimal assembly
- Parses argc/argv/envp from stack
- Calls `_init()` (runs constructors)
- Calls `main(argc, argv, envp)`
- Calls `_fini()` (runs destructors)
- Exits with main's return value

**crt1.c** - Minimal startup (no ctors/dtors)
- Lightweight alternative to crt0.c
- Just calls main() and exit()
- Use when you don't need constructor support

**crti.c** - Initialization prologue
- Defines `_init()` and `_fini()` functions
- Pure C implementation

**crtn.c** - Initialization epilogue
- Completes init/fini functions
- Minimal or empty in C version

**crtbegin.c** - Constructor/destructor support
- Manages `__CTOR_LIST__` and `__DTOR_LIST__`
- Calls all global constructors
- Calls all global destructors

**crtend.c** - List terminators
- Marks end of constructor/destructor lists

## Why C Instead of Assembly?

**Advantages:**
1. **Portable** - Works on any architecture
2. **Maintainable** - Easy to read and modify
3. **Safe** - Compiler handles ABI details
4. **Extensible** - Add new architectures easily

**Only minimal assembly needed:**
- Initial stack setup
- Frame pointer clear
- Stack alignment
- Jump to C code

Everything else is pure C!

## Adding New Architecture

To support a new architecture, edit `crt0.c`:

```c
#elif defined(__YOUR_ARCH__)

void __attribute__((naked, used, section(".text.startup")))
_start(void)
{
    __asm__ volatile (
        "your_asm_to_setup_stack\n"
        "your_asm_to_call _start_c with stack pointer\n"
        "infinite_loop_instruction\n"
        ::: "registers", "memory"
    );
}
#endif
```

The `_start_c()` function is completely portable C code!

## Link Order

Standard link order:
```
crt0.o crti.o crtbegin.o [user_code] [libc.a] crtend.o crtn.o
```

Minimal link order (no ctors/dtors):
```
crt1.o [user_code] [libc.a]
```

## Building

```bash
cd libc/crt
make          # Build all CRT objects
make clean    # Clean
```

Or from userland/:
```bash
make crt      # Build CRT objects
```

## Testing Constructor/Destructor

```c
#include <stdio.h>

__attribute__((constructor))
void my_constructor(void) {
    printf("Constructor called!\n");
}

__attribute__((destructor))
void my_destructor(void) {
    printf("Destructor called!\n");
}

int main(void) {
    printf("Main running\n");
    return 0;
}
```

Output:
```
Constructor called!
Main running
Destructor called!
```

## Stack Layout on Entry

All architectures receive the same stack layout:

```
High Address
+------------------+
| envp[n] = NULL   |
+------------------+
| envp[1]          |
+------------------+
| envp[0]          |
+------------------+
| argv[argc] = NULL|
+------------------+
| argv[argc-1]     |
| ...              |
| argv[1]          |
| argv[0]          |
+------------------+
| argc             | <-- Stack pointer on entry
+------------------+
Low Address
```

The `_start_c()` function parses this in pure C - no assembly needed!

## Choosing crt0 vs crt1

**Use crt0.o when:**
- You need global constructors/destructors
- Using C++ code
- Using GCC `__attribute__((constructor))`
- Want full init/fini support

**Use crt1.o when:**
- Minimal C programs
- No constructors needed
- Want smallest possible startup
- Embedded/bare-metal systems

## Compiler Compatibility

- ✅ GCC
- ✅ Clang
- ✅ Any compiler with `__attribute__` support

The code uses standard C with minimal compiler-specific features.

## Environment Variable

Global `environ` is set automatically:

```c
extern char **environ;

int main(void) {
    for (char **env = environ; *env; env++) {
        printf("%s\n", *env);
    }
    return 0;
}
```

## Thread Safety

Current implementation is single-threaded. For multi-threaded support:
- Make `environ` thread-local
- Protect constructor/destructor lists with mutexes
- Add TLS initialization code

## Future Enhancements

- [ ] Thread-Local Storage (TLS) support
- [ ] Dynamic linker integration
- [ ] Shared library support
- [ ] Position-independent code (PIC)
- [ ] More architectures (MIPS, PowerPC, etc.)
