
#pragma once

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Error handling.
 *-----------------------------------------------------------------------------------------------*/
/** Clean error handling. */
#define Stopif(assertion, error_action, ...)                                                       \
    {                                                                                              \
        if (assertion) {                                                                           \
            fprintf(stderr, __VA_ARGS__);                                                          \
            fprintf(stderr, "\n");                                                                 \
            {                                                                                      \
                error_action;                                                                      \
            }                                                                                      \
        }                                                                                          \
    }

/*-------------------------------------------------------------------------------------------------
 *                         Compare function for soring time_t in ascending order.
 *-----------------------------------------------------------------------------------------------*/
/** Compare \c time_t values which are used as keys in the GLib \c Tree or \c qsort.
 *
 * Dates are sorted in ascending order.
 */
int time_t_compare_func(void const *a, void const *b, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                                          Accumulators
 *-----------------------------------------------------------------------------------------------*/
/** How to accumulate values from a day, eg take the first, last, sum, max, min. */
typedef double (*Accumulator)(double acc, double val);

/** Sum all values to get a total. */
double accum_sum(double acc, double val);

/** Keep the maximum value, ignoring NaNs. */
double accum_max(double acc, double val);

/** Just keep the last value. */
double accum_last(double _acc, double val);

/** Average the values. */
double accum_avg(double acc, double val);

/*-------------------------------------------------------------------------------------------------
 *                                          Converters
 *-----------------------------------------------------------------------------------------------*/
/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

static inline double
kelvin_to_fahrenheit(double k)
{
    double c = (k - 273.15);
    double f = 9.0 / 5.0 * c + 32.0;
    return f;
}

static inline double
change_in_kelvin_to_change_in_fahrenheit(double dk)
{
    return 9.0 / 5.0 * dk;
}

static inline double
id_func(double val)
{
    return val;
}

static inline double
mps_to_mph(double val)
{
    return 2.23694 * val;
}

static inline double
mm_to_in(double val)
{
    return val / 25.4;
}

static inline double
m_to_in(double val)
{
    return val * 39.37008;
}

/** Uppercase a string. */
static inline void
to_uppercase(char string[static 1])
{
    assert(string);

    char *cur = &string[0];
    while (*cur) {
        *cur = toupper(*cur);
        cur++;
    }
}
