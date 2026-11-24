#ifndef ENV_TEXT_H
#define ENV_TEXT_H

#include "io.h"

struct text {
    char *data;
    size_t capacity;
    size_t length;
    bool overflow;
};

static inline void text_byte(struct text *out, char c)
{
    if (out->length + 1 >= out->capacity) {
        out->overflow = true;
        return;
    }
    out->data[out->length++] = c;
    out->data[out->length] = '\0';
}

static inline void text_add(struct text *out, const char *s)
{
    while (*s) {
        text_byte(out, *s++);
    }
}

static inline void text_unsigned(struct text *out, uint32_t n)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + n % 10u);
        n /= 10u;
    } while (n);
    while (count) {
        text_byte(out, digits[--count]);
    }
}

static inline bool text_equal(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static inline bool text_starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s++ != *prefix++) {
            return false;
        }
    }
    return true;
}

#endif
