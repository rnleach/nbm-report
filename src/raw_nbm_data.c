#include <assert.h>
#include <stdbool.h>

#include "raw_nbm_data.h"

struct RawNbmData {
    time_t init_time;
    char *site_id;
    char *site_name;
    char *raw_data;
    size_t raw_data_size;
};

struct RawNbmData *
raw_nbm_data_new(time_t init_time, char *site, char *name, char *text, size_t text_len)
{
    struct RawNbmData *new = malloc(sizeof(struct RawNbmData));
    assert(new);

    *new = (struct RawNbmData){.init_time = init_time,
                               .site_id = site,
                               .site_name = name,
                               .raw_data = text,
                               .raw_data_size = text_len};

    return new;
}

void
raw_nbm_data_free(struct RawNbmData **data)
{
    assert(data);

    struct RawNbmData *ptr = *data;

    if (ptr) {
        free(ptr->site_id);
        free(ptr->site_name);
        free(ptr->raw_data);
        free(ptr);
    }

    *data = 0;
}

time_t
raw_nbm_data_init_time(struct RawNbmData const *data)
{
    return data->init_time;
}

char const *
raw_nbm_data_site_name(struct RawNbmData const *data)
{
    return data->site_name;
}

char const *
raw_nbm_data_site_id(struct RawNbmData const *data)
{
    return data->site_id;
}

char *
raw_nbm_data_text(struct RawNbmData *data)
{
    return data->raw_data;
}

size_t
raw_nbm_data_text_len(struct RawNbmData *data)
{
    return data->raw_data_size;
}
