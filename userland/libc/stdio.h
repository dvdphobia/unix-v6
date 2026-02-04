/* stdio.h - Standard I/O definitions */
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

/* File type */
typedef struct {
    int fd;
    int flags;
    int ungetc_char;
    int has_ungetc;
    char buffer[1024];
    int buf_pos;
    int buf_len;
} FILE;

/* Standard streams */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Constants */
#define EOF (-1)
#define BUFSIZ 1024
#define FOPEN_MAX 20

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* File operations */
FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);
int fflush(FILE *stream);

/* Character I/O */
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
int getchar(void);
int putchar(int c);
int ungetc(int c, FILE *stream);

/* Line I/O */
char *fgets(char *s, int size, FILE *stream);
int fputs(const char *s, FILE *stream);
int puts(const char *s);

/* Block I/O */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* Positioning */
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

/* Error handling */
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);

/* Formatted output */
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

#endif /* _STDIO_H */
