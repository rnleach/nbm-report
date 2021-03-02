#include "nbm.h"

#include "cache.h"
#include "download.h"
#include "nbm_data.h"
#include "site_validation.h"

void
nbm_tools_initialize()
{
    cache_initialize();
    download_module_initialize();
}

void
nbm_tools_finalize()
{
    download_module_finalize();
    cache_finalize();
}

NBMData *
retrieve_data(SiteValidation *validation)
{
    char const *const site = site_validation_site_id_alias(validation);
    char const *const site_nm = site_validation_site_name_alias(validation);
    char const *const file_name = site_validation_file_name_alias(validation);
    time_t init_time = site_validation_init_time(validation);

    RawNbmData *raw_nbm_text = retrieve_data_for_site(site, site_nm, file_name, init_time);
    Stopif(!raw_nbm_text, return 0, "Error retrieving raw text data.");

    NBMData *result = parse_raw_nbm_data(raw_nbm_text);
    raw_nbm_data_free(&raw_nbm_text);
    Stopif(!result, return 0, "Error parsing nbm text data.");

    return result;
}
