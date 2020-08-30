#ifndef _NBM_REPORT_DOWNLOAD_H_
#define _NBM_REPORT_DOWNLOAD_H_

/**
 * Retrieve the CSV data for a site.
 *
 * This function will retrieve the latest available NBM run data for the given site.
 *
 * \param site is the name of the site you want to download data for. It will be modified to force
 * all the characters to upper case.
 *
 * \returns the contents of the downloaded CSV file as a string. If there is an error and it cannot
 * retrieve the data, it returns the null pointer. The returned string is allocated, and you will
 * responsible for freeing it if necessary.
 */
char *retrieve_data_for_site(char site[static 1]);

#endif
