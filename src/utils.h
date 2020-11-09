#pragma once

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Error handling.
 *-----------------------------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------------------------------
 *                         Compare function for soring time_t in ascending order.
 *-----------------------------------------------------------------------------------------------*/
/** Compare \c time_t values which are used as keys in the GLib \c Tree or \c qsort.
 *
 * Dates are sorted in ascending order.
 */
int time_t_compare_func(void const *a, void const *b, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                                          Accumulators
 *-----------------------------------------------------------------------------------------------*/
/** How to accumulate values from a day, eg take the first, last, sum, max, min. */
typedef double (*Accumulator)(double acc, double val);

/** Sum all values to get a total. */
double accum_sum(double acc, double val);

/** Keep the maximum value, ignoring NaNs. */
double accum_max(double acc, double val);

/** Just keep the last value. */
double accum_last(double _acc, double val);

/** Average the values. */
double accum_avg(double acc, double val);

/*-------------------------------------------------------------------------------------------------
 *                                          Converters
 *-----------------------------------------------------------------------------------------------*/
/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

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
 *                                   Resizable Text Buffer
 *-----------------------------------------------------------------------------------------------*/
struct TextBuffer {
    union {
        unsigned char *byte_data;
        char *text_data;
    };

    size_t size;
    size_t capacity;
};

static inline struct TextBuffer
text_buffer_with_capacity(size_t capacity)
{
    unsigned char *buf = 0;
    if (capacity) {
        buf = malloc(capacity);
        assert(buf);
    }

    return (struct TextBuffer){.byte_data = buf, .size = 0, .capacity = capacity};
}

static inline void
text_buffer_clear(struct TextBuffer buf[static 1])
{
    free(buf->byte_data);
    buf->byte_data = 0;
    buf->capacity = 0;
    buf->size = 0;
}

static inline void
text_buffer_set_capacity(struct TextBuffer buf[static 1], size_t new_capacity)
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
text_buffer_append(struct TextBuffer buf[static 1], size_t text_len, char const text[text_len])
{
    size_t new_capacity = buf->size + text_len + 1;

    if (buf->capacity < new_capacity) {
        text_buffer_set_capacity(buf, new_capacity);
    }

    size_t start = buf->size == 0 ? 0 : buf->size - 1;

    memcpy(&(buf->text_data[start]), text, text_len);
    buf->size += buf->size == 0 ? text_len + 1 : text_len;
    buf->text_data[buf->size - 1] = 0;
}

static inline char *
text_buffer_steal_text(struct TextBuffer buf[static 1])
{
    char *out = buf->text_data;
    buf->size = 0;
    buf->capacity = 0;
    buf->text_data = 0;
    return out;
}

static inline bool
text_buffer_is_empty(struct TextBuffer const buf)
{
    return buf.size == 0;
}

/*-------------------------------------------------------------------------------------------------
 *                                   Resizable Byte Buffer
 *-----------------------------------------------------------------------------------------------*/
struct ByteBuffer {
    unsigned char *data;
    size_t size;
    size_t capacity;
};

static inline struct ByteBuffer
byte_buffer_with_capacity(size_t capacity)
{
    unsigned char *buf = 0;
    if (capacity) {
        buf = malloc(capacity);
        assert(buf);
    }

    return (struct ByteBuffer){.data = buf, .size = 0, .capacity = capacity};
}

static inline void
byte_buffer_clear(struct ByteBuffer buf[static 1])
{
    free(buf->data);
    buf->data = 0;
    buf->capacity = 0;
    buf->size = 0;
}

static inline void
byte_buffer_set_capacity(struct ByteBuffer buf[static 1], size_t new_capacity)
{
    assert(new_capacity >= buf->capacity);

    unsigned char *new_buf = realloc(buf->data, new_capacity);
    if (new_capacity) {
        assert(new_buf);
    }

    buf->data = new_buf;
    buf->capacity = new_capacity;
}

static inline void
byte_buffer_increase_size(struct ByteBuffer buf[static 1], size_t size_increase)
{
    size_t new_size = buf->size + size_increase;
    assert(buf->capacity >= new_size);
    buf->size = new_size;
}

static inline size_t
byte_buffer_remaining_capacity(struct ByteBuffer buf[static 1])
{
    return buf->capacity - buf->size;
}

static inline unsigned char *
byte_buffer_next_write_pos(struct ByteBuffer buf[static 1])
{
    return &buf->data[buf->size];
}
