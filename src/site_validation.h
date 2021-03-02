#pragma once

#include "nbm.h"

/*
 * Most of the declarations for this module are exported in the static library via the nbm.h header.
 */

/** Get an alias (pointer) to the file name.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *site_validation_file_name_alias(SiteValidation *validation);
