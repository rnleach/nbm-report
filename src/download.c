#include "download.h"
#include "cache.h"

#include <curl/curl.h>

#define URL_LENGTH 1024

extern bool global_verbose;

/** Format the file name for downloading.
 *
 * If it is a site name then the spaces need to be replaced with the "%20" string.
 */
static void
format_file_name_for_url(int buf_len, char url_file_name[buf_len], char const file_name[static 1])
{
    assert(file_name);

    // Copy into the new string exactly, except replace ' ' by "%20"
    int j = 0;
    for (int i = 0; file_name[i]; i++) {
        Stopif(j == buf_len - 1, exit(EXIT_FAILURE), "URL Buffer overflow");
        if (isspace(file_name[i])) {
            url_file_name[j] = '%';
            url_file_name[j + 1] = '2';
            url_file_name[j + 2] = '0';
            j += 3;
        } else {
            url_file_name[j] = file_name[i];
            j += 1;
        }
    }
    url_file_name[j] = '\0';
}

/**
 * Build the URL to retrieve data from the web into a string.
 *
 * There is a maximum size limit of 255 characters for the URL.
 *
 * \param file_name is a name of the file to download.
 * \param init_time is the model initialization time desired for download.
 *
 * \returns a pointer to a static string that contains the URL. It is important to NOT free this
 * string since it points to static memory.
 */
static char const *
build_download_url(char const file_name[static 1], time_t data_init_time)
{
    assert(file_name);

    static char const *base_url = "https://hwp-viz.gsd.esrl.noaa.gov/wave1d/data/archive/";
    static char url[URL_LENGTH] = {0};
    // Clear the memory before starting over
    memset(url, 0, URL_LENGTH);

    // Format the site for the url.
    char url_file_name[64] = {0};
    format_file_name_for_url(sizeof(url_file_name), url_file_name, file_name);

    struct tm init_time = *gmtime(&data_init_time);

    int year = init_time.tm_year + 1900;
    int month = init_time.tm_mon + 1;
    int day = init_time.tm_mday;
    int hour = init_time.tm_hour;

    // NBM 4.0 started on 9/23/2020
    struct tm nbm_version_4_starts_tm = {.tm_year = 2020 - 1900, .tm_mon = 9 - 1, .tm_mday = 23};
    time_t nbm_version_4_starts = timegm(&nbm_version_4_starts_tm);

    // NBM 4.1 started on 1/11/2023
    struct tm nbm_version_41_starts_tm = {.tm_year = 2023 - 1900, .tm_mon = 1 - 1, .tm_mday = 11};
    time_t nbm_version_41_starts = timegm(&nbm_version_41_starts_tm);

    if (data_init_time > nbm_version_41_starts) {
        // NBM 4.1
        sprintf(url, "%s%4d/%02d/%02d/NBM4.1/%02d/%s", base_url, year, month, day, hour,
                url_file_name);
    } else if (data_init_time > nbm_version_4_starts) {
        // NBM 4.0
        sprintf(url, "%s%4d/%02d/%02d/NBM4.0/%02d/%s", base_url, year, month, day, hour,
                url_file_name);
    } else {
        sprintf(url, "%s%4d/%02d/%02d/NBM/%02d/%s", base_url, year, month, day, hour,
                url_file_name);
    }

    return url;
}

/** Write callback for cURL. */
static size_t
write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct TextBuffer *buf = userp;
    char *text = contents;

    text_buffer_append(buf, realsize, text);

    return realsize;
}

/** Global curl handle. */
static CURL *curl = 0;

static CURL *
download_module_get_curl_handle(struct TextBuffer *buf)
{
    CURLcode res = 0;
    if (!curl) {
        CURLcode err = curl_global_init(CURL_GLOBAL_DEFAULT);
        Stopif(err, goto ERR_RETURN, "Failed to initialize curl");

        curl = curl_easy_init();
        Stopif(!curl, goto ERR_RETURN, "curl_easy_init failed.");

        res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set fail on error.");

        res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the write_callback.");

        res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user agent.");
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the user data.");

    return curl;

ERR_RETURN:
    return 0;
}

struct TextBuffer
download_file(char const file_name[static 1], time_t init_time)
{
    assert(file_name);

    struct TextBuffer buf = text_buffer_with_capacity(0);
    CURL *lcl_curl = 0;

    int res = 1;
    char const *url = 0;

    buf = cache_retrieve(file_name, init_time);
    if (!text_buffer_is_empty(buf)) {
        if (global_verbose)
            printf("Successfully retrieved from the cache: %s\n", file_name);
        return buf;
    }

    lcl_curl = download_module_get_curl_handle(&buf);
    Stopif(!lcl_curl, goto ERR_RETURN, "Error setting up cURL.");

    url = build_download_url(file_name, init_time);
    assert(url);

    res = curl_easy_setopt(lcl_curl, CURLOPT_URL, url);
    Stopif(res, goto ERR_RETURN, "curl_easy_setopt failed to set the url.");
    res = curl_easy_perform(lcl_curl);

    if (res) {
        long response_code = 0;
        int res2 = curl_easy_getinfo(lcl_curl, CURLINFO_RESPONSE_CODE, &response_code);
        Stopif(res2, goto ERR_RETURN, "curl_easy_getinfo failed: %s", curl_easy_strerror(res2));

        if (response_code == 404) {
            res = CURLE_OK; // 0
            if (global_verbose) {
                printf("file not available: %s\n", url);
            }
        }
    }

    Stopif(res, goto ERR_RETURN, "curl_easy_perform failed: %s \n%s", curl_easy_strerror(res), url);

    if (!text_buffer_is_empty(buf)) {
        if (global_verbose)
            printf("Successfully downloaded: %s\n", url);
        int cache_res = cache_add(file_name, init_time, &buf);
        if (cache_res) {
            fprintf(stderr, "Error saving to cache: %s\n", file_name);
        }
    }

    return buf;

ERR_RETURN:
    text_buffer_clear(&buf);
    return buf;
}

RawNbmData *
retrieve_data_for_site(char const site[static 1], char const site_nm[static 1],
                       char const file_name[static 1], time_t init_time)
{
    assert(site);
    assert(file_name);

    struct TextBuffer buf = download_file(file_name, init_time);

    Stopif(text_buffer_is_empty(buf), goto ERR_RETURN, "No data retrieved for %s (%s)", site,
           file_name);

    char *data_site = malloc(strlen(site) + 1);
    strcpy(data_site, site);

    char *data_name = malloc(strlen(site_nm) + 1);
    strcpy(data_name, site_nm);

    return raw_nbm_data_new(init_time, data_site, data_name, buf.text_data, buf.size);

ERR_RETURN:
    text_buffer_clear(&buf);
    return 0;
}

void
download_module_initialize()
{
}

void
download_module_finalize()
{
    if (curl) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}
