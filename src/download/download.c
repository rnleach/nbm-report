#include "download.h"
#include "../utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#define URL_LENGTH 256

/**
 * Build the URL to retrieve data from into a string.
 *
 * There is a maximum size limit of 255 characters for the URL.
 *
 * \param site is a string with the name of the site. It will be modified to force the site name to
 * be uppercase.
 *
 * \returns a pointer to a static string that contains the URL. It is important to NOT free this
 * string since it points to static memory.
 */
static char const *
build_download_url(char site[static 1])
{
    static char const *base_url = "https://hwp-viz.gsd.esrl.noaa.gov/wave1d/data/archive/";
    static char url[URL_LENGTH] = {0};
    memset(url, 0, URL_LENGTH);

    // Force uppercase site names.
    char *cur = &site[0];
    while (*cur) {
        *cur = toupper(*cur);
        cur++;
    }

    time_t now_secs = time(0);
    struct tm *now = gmtime(&now_secs);

    int year = now->tm_year + 1900;
    int month = now->tm_mon + 1;
    int day = now->tm_mday;
    int hour = now->tm_hour;

    if (hour >= 3 && hour < 9) {
        hour = 1;
    } else if (hour >= 9 && hour < 17) {
        hour = 7;
    } else if (hour >= 17 && hour < 22) {
        hour = 13;
    } else {
        hour = 19;
    }

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

char *
retrieve_data_for_site(char site[static 1])
{
    assert(site);

    char const *url = build_download_url(site);
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

    return buf.memory;
}
