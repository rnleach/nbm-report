#pragma once

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

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

inline double
kelvin_to_fahrenheit(double k)
{
    double c = (k - 273.15);
    double f = 9.0 / 5.0 * c + 32.0;
    return f;
}

inline double
change_in_kelvin_to_change_in_fahrenheit(double dk)
{
    return 9.0 / 5.0 * dk;
}

inline double
id_func(double val)
{
    return val;
}

inline double
mps_to_mph(double val)
{
    return 2.23694 * val;
}

inline double
mm_to_in(double val)
{
    return val / 25.4;
}

inline double
m_to_in(double val)
{
    return val * 39.37008;
}

/** Uppercase a string. */
inline void
to_uppercase(char string[static 1])
{
    assert(string);

    char *cur = &string[0];
    while (*cur) {
        *cur = toupper(*cur);
        cur++;
    }
}
