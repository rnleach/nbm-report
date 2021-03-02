#pragma once

#include <assert.h>
#include <ctype.h>
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
 *                                       String Utilities
 *-----------------------------------------------------------------------------------------------*/
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
/** A buffer that tracks it's own size and capacity for storing text data as text or bytes. */
struct TextBuffer {
    union {
        unsigned char *byte_data;
        char *text_data;
    };

    size_t size;     /**< The number of valid values stored in the buffer. */
    size_t capacity; /**< The capacity, the buffer will need grown when this equals size. */
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
    if (new_capacity) {
        unsigned char *new_buf = realloc(buf->byte_data, new_capacity);
        assert(new_buf);

        buf->byte_data = new_buf;
        buf->capacity = new_capacity;
        buf->size = buf->size < new_capacity ? buf->size : new_capacity;
    } else {
        text_buffer_clear(buf);
    }
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
/** Resizable buffer for holding bytes. */
struct ByteBuffer {
    unsigned char *data; /**< Pointer to the data. */
    size_t size;         /**< Size of the valid data in the buffer. */
    size_t capacity;     /**< Capacit of the buffer, it will need resized if same as size. */
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
    if (new_capacity) {
        unsigned char *new_buf = realloc(buf->data, new_capacity);
        assert(new_buf);

        buf->data = new_buf;
        buf->capacity = new_capacity;
        buf->size = buf->size < new_capacity ? buf->size : new_capacity;
    } else {
        byte_buffer_clear(buf);
    }
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
