/* ctype.c - Character classification functions */

int isalpha(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || 
           c == '\r' || c == '\f' || c == '\v';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isprint(int c) {
    return c >= 0x20 && c <= 0x7E;
}

int isgraph(int c) {
    return c >= 0x21 && c <= 0x7E;
}

int iscntrl(int c) {
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

int isxdigit(int c) {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int ispunct(int c) {
    return isprint(c) && !isalnum(c) && !isspace(c);
}

int isascii(int c) {
    return c >= 0 && c <= 127;
}

int toupper(int c) {
    if (islower(c))
        return c - 32;
    return c;
}

int tolower(int c) {
    if (isupper(c))
        return c + 32;
    return c;
}

int toascii(int c) {
    return c & 0x7F;
}
