#pragma once

#include <stdbool.h>

#include <time.h>

/** The results of validating a site. */
typedef struct SiteValidation SiteValidation;

/** Create a \c SiteValidation object by validating \c site as if requested at \c request_time.
 *
 * \param site the site id or name you want to validate.
 * \param request_time perform the validation as if the request were being made at this time.
 *
 * \returns a \c SiteValidation object always. To find out if the validation succeeded or failed,
 * use \c site_validation_failed().
 */
SiteValidation *site_validation_create(char const *site, time_t request_time);

/** Check to see if this validation failed.
 *
 * \returns \c true if the validation failed.
 */
bool site_validation_failed(SiteValidation *validation);

/** Print a message explaining the validation failure.
 *
 * Before using this function you check us \c site_validation_failed() to make sure it actually
 * failed.
 */
void site_validation_print_failure_message(SiteValidation *validation);

/** Get an alias (pointer) to the site name.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *const site_validation_site_name_alias(SiteValidation *validation);

/** Get an alias (pointer) to the site id.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *const site_validation_site_id_alias(SiteValidation *validation);

/** Get the NBM model run time for the validation. */
time_t site_validation_init_time(SiteValidation *validation);

/** Free resources and nullify the object. */
void site_validation_free(SiteValidation **validation);

/** Get an alias (pointer) to the file name.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *site_validation_file_name_alias(SiteValidation *validation);
