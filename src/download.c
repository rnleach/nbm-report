#include "download.h"
#include "raw_nbm_data.h"
#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#define URL_LENGTH 256
#define MAX_VERSIONS_TO_ATTEMP_DOWNLOADING 20

/**
 * Using the current time, calculate the most recent available inititialization time.
 */
static struct tm
calc_init_time(int versions_back)
{

    static int valid_times[] = {1, 7, 13, 19};

    time_t now_secs = time(0);
    struct tm *now = gmtime(&now_secs);

    int hour = now->tm_hour;
    int shift_hours = 0;

    int found = 0;
    while (found < versions_back + 1) {
        int curr_version = hour - shift_hours;
        // Force current version into range 0-23
        if (curr_version < 0) {
            while (curr_version < 0) {
                curr_version += 24;
            }
        }
        for (int i = 0; i < sizeof(valid_times) / sizeof(int); i++) {
            if (curr_version == valid_times[i]) {
                found++;
                break;
            }
        }
        shift_hours++;
    }
    shift_hours--;

    time_t init_secs = now_secs - shift_hours * 3600;
    struct tm *init_time = gmtime(&init_secs);
    init_time->tm_sec = 0;
    init_time->tm_min = 0;

    return *init_time;
}

/**
 * Build the URL to retrieve data from into a string.
 *
 * There is a maximum size limit of 255 characters for the URL.
 *
 * \param site is a string with the name of the site.
 * \param data_init_time is the model initialization time desired for download.
 *
 * \returns a pointer to a static string that contains the URL. It is important to NOT free this
 * string since it points to static memory. It also populates the \c site and \c init_time members
 * of the \c data struct.
 */
static char const *
build_download_url(char const site[static 1], struct tm data_init_time)
{
    static char const *base_url = "https://hwp-viz.gsd.esrl.noaa.gov/wave1d/data/archive/";
    static char url[URL_LENGTH] = {0};
    // Clear the memory before starting over
    memset(url, 0, URL_LENGTH);

    assert(site);

    int year = data_init_time.tm_year + 1900;
    int month = data_init_time.tm_mon + 1;
    int day = data_init_time.tm_mday;
    int hour = data_init_time.tm_hour;

    sprintf(url, "%s%4d/%02d/%02d/NBM/%02d/%s.csv", base_url, year, month, day, hour, site);

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

    // Do not free this unless returning an error, it is encapsulated in the return value.
    char *data_site = malloc(strlen(site) + 1);
    strcpy(data_site, site);
    to_uppercase(data_site);

    CURL *curl = curl_easy_init();
    Stopif(!curl, return 0, "curl_easy_init failed.");

    CURLcode res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    Stopif(res, return 0, "curl_easy_setopt failed to set fail on error.");

    struct buffer buf = {0};
    buf.memory = malloc(1);
    buf.memory[0] = 0;
    buf.size = 0;
    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    Stopif(res, free(buf.memory); return 0, "curl_easy_setopt failed to set the write_callback.");

    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user data.");

    res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user agent.");

    int attempt_number = 0;
    struct tm data_init_time = {0};
    char const *url = 0;
    do {
        data_init_time = calc_init_time(attempt_number);
        url = build_download_url(data_site, data_init_time);
        assert(url);

        attempt_number++;

        res = curl_easy_setopt(curl, CURLOPT_URL, url);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the url.");
        res = curl_easy_perform(curl);
    } while (res && attempt_number <= MAX_VERSIONS_TO_ATTEMP_DOWNLOADING);
    Stopif(res, goto ERR_RETURN, "curl_easy_perform failed: %s", curl_easy_strerror(res));

    printf("Successfully downloaded: %s\n", url);

    curl_easy_cleanup(curl);

    return raw_nbm_data_new(data_init_time, data_site, buf.memory, buf.size);

ERR_RETURN:
    free(buf.memory);
    free(data_site);
    return 0;
}
