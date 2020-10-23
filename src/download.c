#include "download.h"
#include "download_cache.h"
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
static time_t
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

    return timegm(init_time);
}

/** Use heuristics to determine if this is a site identifier or name, and format it for a url.
 *
 * If it is a site identifier, all the letters must be in uppercase. If it is a site name then
 * the spaces need to be replaced with the "%20" string.
 */
static void
format_site_for_url(int buf_len, char url_site[buf_len], char const site[static 1])
{
    bool is_name = false;

    // Check for spaces - if so - it's a name.
    // Check for numbers - if so - it's a site.
    for (char const *c = site; *c; c++) {
        if (isspace(*c)) {
            is_name = true;
            break;
        } else if (isdigit(*c)) {
            break;
        }
    }

    if (is_name) {
        // Copy into the new string exactly, except replace ' ' by "%20"
        int j = 0;
        for (int i = 0; site[i]; i++) {
            if (isspace(site[i])) {
                url_site[j] = '%';
                url_site[j + 1] = '2';
                url_site[j + 2] = '0';
                j += 3;
            } else {
                url_site[j] = site[i];
                j += 1;
            }
        }
    } else {
        strcpy(url_site, site);
        to_uppercase(url_site);
    }
}

/**
 * Build the URL to retrieve data from the web into a string.
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
build_download_url(char const site[static 1], time_t data_init_time)
{
    static char const *base_url = "https://hwp-viz.gsd.esrl.noaa.gov/wave1d/data/archive/";
    static char url[URL_LENGTH] = {0};
    // Clear the memory before starting over
    memset(url, 0, URL_LENGTH);

    // Format the site for the url.
    char url_site[64] = {0};
    format_site_for_url(sizeof(url_site), url_site, site);

    assert(site);

    struct tm init_time = *gmtime(&data_init_time);

    int year = init_time.tm_year + 1900;
    int month = init_time.tm_mon + 1;
    int day = init_time.tm_mday;
    int hour = init_time.tm_hour;

    sprintf(url, "%s%4d/%02d/%02d/NBM4.0/%02d/%s.csv", base_url, year, month, day, hour, url_site);

    return url;
}

static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct Buffer *buf = userp;
    char *text = contents;

    buffer_append_text(buf, realsize, text);

    return realsize;
}

static CURL *curl = 0;

static CURL *
download_module_get_curl_handle(struct Buffer *buf)
{
    if (!curl) {
        CURLcode err = curl_global_init(CURL_GLOBAL_DEFAULT);
        Stopif(err, exit(EXIT_FAILURE), "Failed to initialize curl");

        curl = curl_easy_init();
        Stopif(!curl, goto ERR_RETURN, "curl_easy_init failed.");

        CURLcode res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set fail on error.");

        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the write_callback.");

        res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user data.");

        res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user agent.");
    }

    return curl;

ERR_RETURN:
    return 0;
};

struct RawNbmData *
retrieve_data_for_site(char const site[static 1])
{
    assert(site);

    // Allocated memory, don't free these unless there is an error.
    char *data_site = malloc(strlen(site) + 1);
    struct Buffer buf = buffer_with_capacity(1024);

    // Keep a copy of the site and force it to upper case.
    strcpy(data_site, site);
    to_uppercase(data_site);

    int res = 1;
    int attempt_number = 0;
    time_t data_init_time = 0;
    char const *url = 0;
    do {
        data_init_time = calc_init_time(attempt_number);

        char *cache_data = download_cache_retrieve(data_site, data_init_time);
        if (cache_data) {
            printf("Successfully retrieved from the cache.\n");
            int cache_data_size = strlen(cache_data) + 1;
            return raw_nbm_data_new(data_init_time, data_site, cache_data, cache_data_size);
        }

        CURL *lcl_curl = download_module_get_curl_handle(&buf);
        Stopif(!lcl_curl, goto ERR_RETURN, "Error setting up cURL.");

        url = build_download_url(site, data_init_time);
        assert(url);

        attempt_number++;

        res = curl_easy_setopt(lcl_curl, CURLOPT_URL, url);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the url.");
        res = curl_easy_perform(lcl_curl);

        if (!res) {
            printf("Successfully downloaded: %s\n", url);
            int cache_res = download_cache_add(data_site, data_init_time, &buf);
            Stopif(cache_res, /* do nothing, just print message */, "Error saving to cache.");
        }

    } while (res && attempt_number <= MAX_VERSIONS_TO_ATTEMP_DOWNLOADING);
    Stopif(res, goto ERR_RETURN, "curl_easy_perform failed: %s", curl_easy_strerror(res));

    curl_easy_cleanup(curl);

    return raw_nbm_data_new(data_init_time, data_site, buf.text_data, buf.size);

ERR_RETURN:
    buffer_clear(&buf);
    free(data_site);
    return 0;
}

void
download_module_initialize()
{
    download_cache_initialize();
}

void
download_module_finalize()
{
    if (curl) {
        curl_global_cleanup();
    }

    download_cache_finalize();
}
