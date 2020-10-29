#pragma once

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Clean error handling. */
#define Stopif(assertion, error_action, ...)                                                       \
    {                                                                                              \
        if (assertion) {                                                                           \
            fprintf(stderr, __VA_ARGS__);                                                          \
            fprintf(stderr, "\n");                                                                 \
            {                                                                                      \
                error_action;                                                                      \
            }                                                                                      \
        }                                                                                          \
    }

static inline double
kelvin_to_fahrenheit(double k)
{
    double c = (k - 273.15);
    double f = 9.0 / 5.0 * c + 32.0;
    return f;
}

static inline double
change_in_kelvin_to_change_in_fahrenheit(double dk)
{
    return 9.0 / 5.0 * dk;
}

static inline double
id_func(double val)
{
    return val;
}

static inline double
mps_to_mph(double val)
{
    return 2.23694 * val;
}

static inline double
mm_to_in(double val)
{
    return val / 25.4;
}

static inline double
m_to_in(double val)
{
    return val * 39.37008;
}

/** Uppercase a string. */
static inline void
to_uppercase(char string[static 1])
{
    assert(string);

    char *cur = &string[0];
    while (*cur) {
        *cur = toupper(*cur);
        cur++;
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                  Resizable Buffer Type
 *-----------------------------------------------------------------------------------------------*/
struct Buffer {
    union {
        unsigned char *byte_data;
        char *text_data;
    };

    size_t size;
    size_t capacity;
};

static inline struct Buffer
buffer_with_capacity(size_t capacity)
{
    unsigned char *buf = 0;
    if (capacity) {
        buf = malloc(capacity);
        assert(buf);
    }

    return (struct Buffer){.byte_data = buf, .size = 0, .capacity = capacity};
}

static inline void
buffer_clear(struct Buffer buf[static 1])
{
    free(buf->byte_data);
    buf->byte_data = 0;
    buf->capacity = 0;
    buf->size = 0;
}

static inline void
buffer_set_capacity(struct Buffer buf[static 1], size_t new_capacity)
{
    unsigned char *new_buf = realloc(buf->byte_data, new_capacity);
    if (new_capacity) {
        assert(new_buf);
    }

    buf->byte_data = new_buf;
    buf->capacity = new_capacity;
    buf->size = buf->size < new_capacity ? buf->size : new_capacity;
}

static inline void
buffer_append_text(struct Buffer buf[static 1], size_t text_len, char const text[text_len])
{
    size_t new_capacity = buf->size + text_len + 1;

    if (buf->capacity < new_capacity) {
        buffer_set_capacity(buf, new_capacity);
    }

    size_t start = buf->size == 0 ? 0 : buf->size - 1;

    memcpy(&(buf->text_data[start]), text, text_len);
    buf->size += buf->size == 0 ? text_len + 1 : text_len;
    buf->text_data[buf->size - 1] = 0;
}

static inline char *
buffer_steal_text(struct Buffer buf[static 1])
{
    char *out = buf->text_data;
    buf->size = 0;
    buf->capacity = 0;
    buf->text_data = 0;
    return out;
}
