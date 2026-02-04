/* stdlib.h - Standard library definitions */
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

/* Memory allocation */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/* Process control */
void exit(int status) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));

/* String conversion */
int atoi(const char *nptr);
long atol(const char *nptr);

/* Math */
int abs(int n);
long labs(long n);

/* Environment */
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

/* Constants */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define NULL ((void *)0)

#endif /* _STDLIB_H */
