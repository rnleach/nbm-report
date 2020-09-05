#include "download.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#define URL_LENGTH 256

struct RawNbmData {
    struct tm init_time;
    char *site;
    char *raw_data;
    size_t raw_data_size;
};

void
free_raw_nbm_data(struct RawNbmData **data)
{
    assert(data);

    struct RawNbmData *ptr = *data;

    if (ptr) {
        free(ptr->site);
        free(ptr->raw_data);
        free(ptr);
    }

    *data = 0;
}

struct tm
raw_nbm_data_init_time(struct RawNbmData *data)
{
    return data->init_time;
}

char const *
raw_nbm_data_site(struct RawNbmData *data)
{
    return data->site;
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

/**
 * Using the current time, calculate the most recent available inititialization time.
 */
static struct tm
calc_init_time()
{

    time_t now_secs = time(0);
    struct tm *now = gmtime(&now_secs);

    int hour = now->tm_hour;
    int shift_hours = 0;

    if (hour >= 4 && hour < 9) {
        shift_hours = 1 - hour;
    } else if (hour >= 9 && hour < 17) {
        shift_hours = 7 - hour;
    } else if (hour >= 17 && hour < 22) {
        shift_hours = 13 - hour;
    } else {
        if (hour < 4) {
            shift_hours = -24 + 19 - hour;
        } else {
            shift_hours = 19 - hour;
        }
    }
    time_t init_secs = now_secs + shift_hours * 3600;
    struct tm *init_time = gmtime(&init_secs);
    init_time->tm_sec = 0;
    init_time->tm_min = 0;

    return *init_time;
}

/** Uppercase a string. */
static void
to_uppercase(char string[static 1])
{
    assert(string);

    char *cur = &string[0];
    while (*cur) {
        *cur = toupper(*cur);
        cur++;
    }
}

/**
 * Build the URL to retrieve data from into a string.
 *
 * There is a maximum size limit of 255 characters for the URL.
 *
 * \param site is a string with the name of the site.
 *
 * \param data is a struct that contains metadata about the downloaded data. Some of that metadata
 * is populated by this function including the site (to all upper case letters) and the
 * initialization time of the the NBM run corresponding to this URL.
 *
 * \returns a pointer to a static string that contains the URL. It is important to NOT free this
 * string since it points to static memory. It also populates the \c site and \c init_time members
 * of the \c data struct.
 */
static char const *
build_download_url(char const site[static 1], struct RawNbmData *data)
{
    static char const *base_url = "https://hwp-viz.gsd.esrl.noaa.gov/wave1d/data/archive/";
    static char url[URL_LENGTH] = {0};
    // Clear the memory before starting over
    memset(url, 0, URL_LENGTH);

    data->site = malloc(strlen(site) + 1);
    strcpy(data->site, site);
    to_uppercase(data->site);

    data->init_time = calc_init_time();

    int year = data->init_time.tm_year + 1900;
    int month = data->init_time.tm_mon + 1;
    int day = data->init_time.tm_mday;
    int hour = data->init_time.tm_hour;

    sprintf(url, "%s%4d/%02d/%02d/NBM/%02d/%s.csv", base_url, year, month, day, hour, data->site);

    return url;
}

struct buffer {
    char *memory;
    size_t size;
};

static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct buffer *mem = userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    Stopif(!ptr, exit(EXIT_FAILURE), "Out of memory.");

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

struct RawNbmData *
retrieve_data_for_site(char const site[static 1])
{
    assert(site);

    struct RawNbmData *data = calloc(1, sizeof(struct RawNbmData));
    assert(data);

    char const *url = build_download_url(site, data);
    assert(url);

    CURL *curl = curl_easy_init();
    Stopif(!curl, return 0, "curl_easy_init failed.");

    CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url);
    Stopif(res, return 0, "curl_easy_setopt failed to set the url.");

    struct buffer buf = {0};
    buf.memory = malloc(1);
    buf.memory[0] = 0;
    buf.size = 0;
    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    Stopif(res, free(buf.memory); return 0, "curl_easy_setopt failed to set the write_callback.");

    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    Stopif(res, free(buf.memory); return 0, "curl_easy_setopt failed to set the user data.");

    res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    Stopif(res, free(buf.memory); return 0, "curl_easy_setopt failed to set the user agent.");

    res = curl_easy_perform(curl);
    Stopif(res, free(buf.memory);
           return 0, "curl_easy_perform failed: %s", curl_easy_strerror(res));

    curl_easy_cleanup(curl);

    data->raw_data = buf.memory;
    data->raw_data_size = buf.size;

    return data;
}
