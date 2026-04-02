#include <stddef.h>
#include <stdint.h>

void *memset(void *dest, int ch, size_t count) {
    uint8_t *p = (uint8_t *)dest;
    for (size_t i = 0; i < count; i++) {
        p[i] = (uint8_t)ch;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) {
        p++;
    }
    return (size_t)(p - s);
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t len;
    for (len = 0; len < maxlen && s[len] != '\0'; len++)
        ;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *p = dest;
    while ((*p++ = *src++)) { }
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

size_t strlcpy(char *dest, const char *src, size_t size) {
    size_t i;
    if (size == 0) return strlen(src);

    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';

    return strlen(src);
}