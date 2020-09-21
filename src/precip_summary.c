#include "precip_summary.h"
#include "nbm_data.h"
#include "summarize.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                            Liquid Summary
 *-----------------------------------------------------------------------------------------------*/
struct PrecipSummary {
    double precip;
    double prob_001;
    double prob_005;
    double prob_010;
    double prob_025;
    double prob_050;
    double prob_075;
    double prob_100;
    double prob_150;
    double prob_200;
};

static void
show_liquid_summary(struct NBMData const *nbm)
{
    // TODO implement
    assert(false);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Snow Summary
 *-----------------------------------------------------------------------------------------------*/

static void
show_snow_summary(struct NBMData const *nbm)
{
    // TODO implement
    assert(false);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Public API
 *-----------------------------------------------------------------------------------------------*/
void
show_precip_summary(struct NBMData const *nbm, char type)
{
    assert(type == 'r' || type == 's');

    switch (type) {
    case 'r':
        show_liquid_summary(nbm);
        break;
    case 's':
        show_snow_summary(nbm);
        break;
    default:
        assert(false);
    }
}
