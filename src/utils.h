
#ifndef _NBM_REPORT_UTILS_H_
#define _NBM_REPORT_UTILS_H_

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
dmax(double v1, double v2)
{
    return v1 > v2 ? v1 : v2;
}

#endif
