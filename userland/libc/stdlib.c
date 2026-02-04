/* stdlib.c - POSIX standard library functions */
#include <stddef.h>

extern void *sbrk(int incr);
extern int brk(void *addr);
extern void exit(int status);

/* Simple malloc implementation using sbrk */
typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

static block_t *free_list = NULL;

#define BLOCK_SIZE sizeof(block_t)

block_t *find_free_block(block_t **last, size_t size) {
    block_t *current = free_list;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

block_t *request_space(block_t *last, size_t size) {
    block_t *block;
    block = sbrk(0);
    void *request = sbrk(size + BLOCK_SIZE);
    if (request == (void *)-1)
        return NULL;
    
    if (last)
        last->next = block;
    
    block->size = size;
    block->free = 0;
    block->next = NULL;
    return block;
}

void *malloc(size_t size) {
    block_t *block;
    
    if (size <= 0)
        return NULL;
    
    if (!free_list) {
        block = request_space(NULL, size);
        if (!block)
            return NULL;
        free_list = block;
        return (block + 1);
    }
    
    block_t *last = free_list;
    block = find_free_block(&last, size);
    
    if (block) {
        block->free = 0;
        return (block + 1);
    }
    
    block = request_space(last, size);
    if (!block)
        return NULL;
    
    return (block + 1);
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        char *p = ptr;
        while (total--)
            *p++ = 0;
    }
    return ptr;
}

block_t *get_block_ptr(void *ptr) {
    return (block_t *)ptr - 1;
}

void free(void *ptr) {
    if (!ptr)
        return;
    
    block_t *block = get_block_ptr(ptr);
    block->free = 1;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr)
        return malloc(size);
    
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    block_t *block = get_block_ptr(ptr);
    if (block->size >= size)
        return ptr;
    
    void *new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;
    
    /* Copy old data */
    char *src = ptr;
    char *dst = new_ptr;
    size_t copy_size = block->size < size ? block->size : size;
    while (copy_size--)
        *dst++ = *src++;
    
    free(ptr);
    return new_ptr;
}

int atoi(const char *nptr) {
    int result = 0;
    int sign = 1;
    
    /* Skip whitespace */
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n')
        nptr++;
    
    /* Handle sign */
    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }
    
    /* Convert digits */
    while (*nptr >= '0' && *nptr <= '9') {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }
    
    return sign * result;
}

long atol(const char *nptr) {
    long result = 0;
    int sign = 1;
    
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n')
        nptr++;
    
    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }
    
    while (*nptr >= '0' && *nptr <= '9') {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }
    
    return sign * result;
}

void abort(void) {
    exit(134); /* SIGABRT */
}

int abs(int n) {
    return n < 0 ? -n : n;
}

long labs(long n) {
    return n < 0 ? -n : n;
}

/* Environment variables (simple implementation) */
static char *environ_storage[256];
static char **environ = environ_storage;
static int environ_count = 0;

char *getenv(const char *name) {
    int i;
    int name_len = 0;
    
    while (name[name_len] && name[name_len] != '=')
        name_len++;
    
    for (i = 0; i < environ_count; i++) {
        if (!environ[i])
            continue;
        
        int j = 0;
        while (j < name_len && environ[i][j] == name[j])
            j++;
        
        if (j == name_len && environ[i][j] == '=')
            return &environ[i][j + 1];
    }
    
    return NULL;
}

int setenv(const char *name, const char *value, int overwrite) {
    /* Find if exists */
    int name_len = 0;
    while (name[name_len] && name[name_len] != '=')
        name_len++;
    
    for (int i = 0; i < environ_count; i++) {
        if (!environ[i])
            continue;
        
        int j = 0;
        while (j < name_len && environ[i][j] == name[j])
            j++;
        
        if (j == name_len && environ[i][j] == '=') {
            if (!overwrite)
                return 0;
            
            /* Replace */
            size_t len = name_len + 1 + 0;
            const char *v = value;
            while (*v++) len++;
            
            char *new_env = malloc(len + 1);
            if (!new_env)
                return -1;
            
            char *p = new_env;
            for (int k = 0; k < name_len; k++)
                *p++ = name[k];
            *p++ = '=';
            while (*value)
                *p++ = *value++;
            *p = '\0';
            
            environ[i] = new_env;
            return 0;
        }
    }
    
    /* Add new */
    if (environ_count >= 255)
        return -1;
    
    size_t len = name_len + 1 + 0;
    const char *v = value;
    while (*v++) len++;
    
    char *new_env = malloc(len + 1);
    if (!new_env)
        return -1;
    
    char *p = new_env;
    for (int k = 0; k < name_len; k++)
        *p++ = name[k];
    *p++ = '=';
    while (*value)
        *p++ = *value++;
    *p = '\0';
    
    environ[environ_count++] = new_env;
    environ[environ_count] = NULL;
    
    return 0;
}

int unsetenv(const char *name) {
    int name_len = 0;
    while (name[name_len] && name[name_len] != '=')
        name_len++;
    
    for (int i = 0; i < environ_count; i++) {
        if (!environ[i])
            continue;
        
        int j = 0;
        while (j < name_len && environ[i][j] == name[j])
            j++;
        
        if (j == name_len && environ[i][j] == '=') {
            /* Shift remaining entries */
            for (int k = i; k < environ_count - 1; k++)
                environ[k] = environ[k + 1];
            environ_count--;
            environ[environ_count] = NULL;
            return 0;
        }
    }
    
    return 0;
}
