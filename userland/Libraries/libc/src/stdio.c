/* stdio.c - POSIX standard I/O functions */
#include <stddef.h>
#include <stdarg.h>

extern int read(int fd, void *buf, int count);
extern int write(int fd, const void *buf, int count);
extern int open(const char *path, int mode);
extern int close(int fd);
extern long lseek(int fd, long offset, int whence);
extern size_t strlen(const char *s);

/* File structure */
#define FOPEN_MAX 20
#define BUFSIZ 1024
#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* File flags */
#define _FILE_READ  0x01
#define _FILE_WRITE 0x02
#define _FILE_EOF   0x04
#define _FILE_ERR   0x08
#define _FILE_UNBUF 0x10

typedef struct {
    int fd;
    int flags;
    int ungetc_char;
    int has_ungetc;
    char buffer[BUFSIZ];
    int buf_pos;
    int buf_len;
} FILE;

/* Forward declaration - after FILE is defined */
int fflush(FILE *stream);

static FILE file_table[FOPEN_MAX];
static FILE stdin_file = {0, _FILE_READ, -1, 0, {0}, 0, 0};
static FILE stdout_file = {1, _FILE_WRITE | _FILE_UNBUF, -1, 0, {0}, 0, 0};
static FILE stderr_file = {2, _FILE_WRITE | _FILE_UNBUF, -1, 0, {0}, 0, 0};

FILE *stdin = &stdin_file;
FILE *stdout = &stdout_file;
FILE *stderr = &stderr_file;

void __stdio_init(void) {
    for (int i = 0; i < FOPEN_MAX; i++)
        file_table[i].fd = -1;
}

FILE *fopen(const char *pathname, const char *mode) {
    int flags = 0;
    int fd;
    
    /* Parse mode */
    if (mode[0] == 'r') {
        flags = 0; /* O_RDONLY */
    } else if (mode[0] == 'w') {
        flags = 0x201; /* O_WRONLY | O_CREAT | O_TRUNC */
    } else if (mode[0] == 'a') {
        flags = 0x401; /* O_WRONLY | O_CREAT | O_APPEND */
    } else {
        return NULL;
    }
    
    if (mode[1] == '+' || (mode[1] && mode[2] == '+'))
        flags = (flags & ~0x03) | 0x02; /* O_RDWR */
    
    fd = open(pathname, flags);
    if (fd < 0)
        return NULL;
    
    /* Find free slot */
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (file_table[i].fd < 0) {
            file_table[i].fd = fd;
            file_table[i].flags = (mode[0] == 'r') ? _FILE_READ : _FILE_WRITE;
            file_table[i].has_ungetc = 0;
            file_table[i].buf_pos = 0;
            file_table[i].buf_len = 0;
            return &file_table[i];
        }
    }
    
    close(fd);
    return NULL;
}

int fclose(FILE *stream) {
    if (!stream || stream->fd < 0)
        return EOF;
    
    fflush(stream);
    int ret = close(stream->fd);
    stream->fd = -1;
    return ret < 0 ? EOF : 0;
}

int fflush(FILE *stream) {
    if (!stream)
        return 0;
    
    if ((stream->flags & _FILE_WRITE) && stream->buf_len > 0) {
        int written = write(stream->fd, stream->buffer, stream->buf_len);
        if (written < 0)
            return EOF;
        stream->buf_len = 0;
        stream->buf_pos = 0;
    }
    
    return 0;
}

int fgetc(FILE *stream) {
    if (!stream || !(stream->flags & _FILE_READ))
        return EOF;
    
    if (stream->has_ungetc) {
        stream->has_ungetc = 0;
        return stream->ungetc_char;
    }
    
    if (stream->buf_pos >= stream->buf_len) {
        stream->buf_len = read(stream->fd, stream->buffer, BUFSIZ);
        if (stream->buf_len <= 0) {
            stream->flags |= (stream->buf_len == 0) ? _FILE_EOF : _FILE_ERR;
            return EOF;
        }
        stream->buf_pos = 0;
    }
    
    return (unsigned char)stream->buffer[stream->buf_pos++];
}

int fputc(int c, FILE *stream) {
    if (!stream || !(stream->flags & _FILE_WRITE))
        return EOF;
    
    if (stream->flags & _FILE_UNBUF) {
        char ch = (char)c;
        return write(stream->fd, &ch, 1) == 1 ? c : EOF;
    }
    
    stream->buffer[stream->buf_len++] = (char)c;
    
    if (stream->buf_len >= BUFSIZ || c == '\n') {
        if (fflush(stream) == EOF)
            return EOF;
    }
    
    return c;
}

char *fgets(char *s, int size, FILE *stream) {
    if (!s || size <= 0 || !stream)
        return NULL;
    
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (i == 0)
                return NULL;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n')
            break;
    }
    s[i] = '\0';
    return s;
}

int fputs(const char *s, FILE *stream) {
    while (*s) {
        if (fputc(*s++, stream) == EOF)
            return EOF;
    }
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    char *p = ptr;
    size_t total = size * nmemb;
    size_t count = 0;
    
    for (size_t i = 0; i < total; i++) {
        int c = fgetc(stream);
        if (c == EOF)
            break;
        p[i] = (char)c;
        count++;
    }
    
    return count / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    const char *p = ptr;
    size_t total = size * nmemb;
    size_t count = 0;
    
    for (size_t i = 0; i < total; i++) {
        if (fputc(p[i], stream) == EOF)
            break;
        count++;
    }
    
    return count / size;
}

int fseek(FILE *stream, long offset, int whence) {
    if (!stream)
        return -1;
    
    fflush(stream);
    stream->buf_pos = 0;
    stream->buf_len = 0;
    
    return lseek(stream->fd, offset, whence) < 0 ? -1 : 0;
}

long ftell(FILE *stream) {
    if (!stream)
        return -1;
    
    long pos = lseek(stream->fd, 0, SEEK_CUR);
    if (pos < 0)
        return -1;
    
    return pos - (stream->buf_len - stream->buf_pos);
}

int feof(FILE *stream) {
    return stream ? (stream->flags & _FILE_EOF) != 0 : 0;
}

int ferror(FILE *stream) {
    return stream ? (stream->flags & _FILE_ERR) != 0 : 0;
}

void clearerr(FILE *stream) {
    if (stream)
        stream->flags &= ~(_FILE_EOF | _FILE_ERR);
}

int ungetc(int c, FILE *stream) {
    if (!stream || c == EOF || stream->has_ungetc)
        return EOF;
    
    stream->ungetc_char = c;
    stream->has_ungetc = 1;
    stream->flags &= ~_FILE_EOF;
    return c;
}

/* Formatted output helpers */
static void print_num(char *buf, long num, int base, int is_signed) {
    char tmp[32];
    int i = 0;
    int neg = 0;
    
    if (is_signed && num < 0) {
        neg = 1;
        num = -num;
    }
    
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num > 0) {
            int digit = num % base;
            tmp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            num /= base;
        }
    }
    
    if (neg)
        tmp[i++] = '-';
    
    /* Reverse */
    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    size_t pos = 0;
    
    while (*format && pos < size - 1) {
        if (*format != '%') {
            str[pos++] = *format++;
            continue;
        }
        
        format++; /* Skip % */
        
        /* Flags */
        int zero_pad = 0;
        int long_arg = 0;
        int width = 0;
        
        while (*format == '0') {
            zero_pad = 1;
            format++;
        }
        
        /* Width */
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }
        
        /* Length modifiers */
        if (*format == 'l') {
            long_arg = 1;
            format++;
            if (*format == 'l') { /* Handle 'll' but treat as long for now in 32-bit */
                format++; 
            }
        }
        
        /* Specifiers */
        char buf[32];
        
        if (*format == 'd' || *format == 'i') {
            long num;
            if (long_arg) num = va_arg(ap, long);
            else num = va_arg(ap, int);
            
            print_num(buf, num, 10, 1);
            
            /* Padding */
            int len = 0;
            while (buf[len]) len++;
            
            while (width > len && pos < size - 1) {
                str[pos++] = zero_pad ? '0' : ' ';
                width--;
            }
            
            for (char *p = buf; *p && pos < size - 1; p++)
                str[pos++] = *p;
            format++;
        } else if (*format == 'u') {
            unsigned long num;
            if (long_arg) num = va_arg(ap, unsigned long);
            else num = va_arg(ap, unsigned int);
            
            print_num(buf, num, 10, 0);
            
            int len = 0;
            while (buf[len]) len++;
            
            while (width > len && pos < size - 1) {
                str[pos++] = zero_pad ? '0' : ' ';
                width--;
            }
            
            for (char *p = buf; *p && pos < size - 1; p++)
                str[pos++] = *p;
            format++;
        } else if (*format == 'x' || *format == 'X') {
            unsigned long num;
            if (long_arg) num = va_arg(ap, unsigned long);
            else num = va_arg(ap, unsigned int);
            
            print_num(buf, num, 16, 0);
            
            int len = 0;
            while (buf[len]) len++;
            
            while (width > len && pos < size - 1) {
                str[pos++] = zero_pad ? '0' : ' ';
                width--;
            }
            
            for (char *p = buf; *p && pos < size - 1; p++)
                str[pos++] = *p;
            format++;
        } else if (*format == 'p') {
            unsigned long num = (unsigned long)va_arg(ap, void *);
            str[pos++] = '0';
            str[pos++] = 'x';
            print_num(buf, num, 16, 0);
            for (char *p = buf; *p && pos < size - 1; p++)
                str[pos++] = *p;
            format++;
        } else if (*format == 's') {
            char *s = va_arg(ap, char *);
            if (!s) s = "(null)";
            
            /* Calculate string length for padding */
            int len = 0;
            while (s[len]) len++;
            
            /* Right align if width specified (standard behavior is left align for most, 
               but usually %Xs is right aligned, %-Xs is left. We don't support - yet.)
               Defaulting to right align like numbers for now as it's common request. */
            while (width > len && pos < size - 1) {
                str[pos++] = ' ';
                width--;
            }
            
            while (*s && pos < size - 1)
                str[pos++] = *s++;
            format++;
        } else if (*format == 'c') {
            str[pos++] = (char)va_arg(ap, int);
            format++;
        } else if (*format == '%') {
             str[pos++] = '%';
             format++;
        } else {
            /* Unknown specifier, behave like literal % */
            /* We already consumed %, so just print the char */
            str[pos++] = *format++;
        }
    }
    
    str[pos] = '\0';
    return pos;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, 10000, format, ap);
    va_end(ap);
    return ret;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, ap);
    return fputs(buf, stream);
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
    return ret;
}

int puts(const char *s) {
    fputs(s, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    return 0;
}

int putchar(int c) {
    return fputc(c, stdout);
}

int getchar(void) {
    return fgetc(stdin);
}
