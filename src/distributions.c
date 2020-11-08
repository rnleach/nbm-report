#include "distributions.h"

#include <math.h>

#include <glib.h>
/*-------------------------------------------------------------------------------------------------
 *                                    CDF implementations.
 *-----------------------------------------------------------------------------------------------*/
struct Percentile {
    double pct;
    double val;
};

static int
percentile_compare(void const *a, void const *b)
{
    struct Percentile const *pa = a;
    struct Percentile const *pb = b;

    if (pa->pct < pb->pct)
        return -1;
    if (pa->pct > pb->pct)
        return 1;
    return 0;
}

struct CumulativeDistribution {
    double quantile_mapped_value;
    struct Percentile *percentiles;
    int size;
    int capacity;
    bool sorted;
};

static struct CumulativeDistribution *
cumulative_dist_new()
{
    struct CumulativeDistribution *new = calloc(1, sizeof(struct CumulativeDistribution));
    assert(new);

    new->percentiles = calloc(10, sizeof(struct Percentile));
    assert(new->percentiles);
    new->capacity = 10;
    new->quantile_mapped_value = NAN;
    new->sorted = false;

    return new;
}

static bool
cumulative_dist_append_pair(struct CumulativeDistribution *dist, double percentile, double value)
{
    assert(dist->capacity >= dist->size);

    // Expand storage if needed.
    if (dist->capacity == dist->size) {
        assert(dist->capacity == 10);
        dist->percentiles = realloc(dist->percentiles, 100 * sizeof(struct Percentile));
        assert(dist->percentiles);
        dist->capacity = 100;
    }

    dist->sorted = false;
    dist->percentiles[dist->size] = (struct Percentile){.pct = percentile, .val = value};
    dist->size++;

    return true;
}

GTree *
extract_cdfs(NBMData const *nbm, char const *cdf_col_name_format, char const *pm_col_name,
             Converter convert)
{
    GTree *cdfs = g_tree_new_full(time_t_compare_func, 0, free, cumulative_dist_free);

    char col_name[256] = {0};
    for (int i = 1; i <= 99; i++) {
        int num_chars = snprintf(col_name, sizeof(col_name), cdf_col_name_format, i);
        Stopif(num_chars >= sizeof(col_name), goto ERR_RETURN,
               "error with snprintf, buffer too small.");

        // Create an iterator, if the pointer is 0, no such column, continue.
        NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
        if (!it) {
            // This column doesn't exist, so skip it!
            continue;
        }

        struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
        while (view.valid_time && view.value) {

            struct CumulativeDistribution *cd = g_tree_lookup(cdfs, view.valid_time);
            if (!cd) {
                time_t *key = malloc(sizeof(time_t));
                *key = *view.valid_time;
                cd = cumulative_dist_new();
                g_tree_insert(cdfs, key, cd);
            }

            cumulative_dist_append_pair(cd, i + 0.0, convert(*view.value));

            view = nbm_data_row_iterator_next(it);
        }

        nbm_data_row_iterator_free(&it);
    }

    // Get the probability matched info.
    NBMDataRowIterator *it = nbm_data_rows(nbm, pm_col_name);
    if (it) {
        struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
        while (view.valid_time && view.value) {

            struct CumulativeDistribution *cd = g_tree_lookup(cdfs, view.valid_time);
            // Skip times we don't have probability information for.
            if (cd) {
                cd->quantile_mapped_value = convert(*view.value);
            }

            view = nbm_data_row_iterator_next(it);
        }

        nbm_data_row_iterator_free(&it);
    }

    return cdfs;

ERR_RETURN:
    g_tree_unref(cdfs);
    return 0;
}

double
cumulative_dist_pm_value(struct CumulativeDistribution const *ptr)
{
    return ptr->quantile_mapped_value;
}

static void
cumulative_dist_sort(struct CumulativeDistribution *ptr)
{
    qsort(ptr->percentiles, ptr->size, sizeof(struct Percentile), percentile_compare);
    ptr->sorted = true;
}

double
interpolate_prob_of_exceedance(struct CumulativeDistribution *cdf, double target_val)
{
    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }

    struct Percentile *ps = cdf->percentiles;
    int sz = cdf->size;

    assert(sz > 1);

    // Bracket the target value
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].val <= target_val && ps[i].val >= target_val) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket the target value, then 100% of data was below value and the probability
    // of exceedance is 0, or the first percentile marker we had was greater than the target value
    // and and the probability of exceedance is 100.0
    if (left == right) {
        if (ps[0].val >= target_val) {
            return 100.0;
        } else {
            return 0.0;
        }
    }

    double left_x = ps[left].val;
    double right_x = ps[right].val;
    double left_y = ps[left].pct;
    double right_y = ps[right].pct;

    double rise = right_y - left_y;
    double run = right_x - left_x;
    assert(run > 0.0);
    double slope = rise / run;
    double cdf_val = slope * (target_val - left_x) + left_y;

    double prob_exc = 100.0 - cdf_val;
    assert(prob_exc >= 0.0);
    assert(prob_exc <= 100.0);
    return prob_exc;
}

double
cumulative_dist_percentile_value(struct CumulativeDistribution *cdf, double target_percentile)
{
    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }
    struct Percentile *ps = cdf->percentiles;
    int sz = cdf->size;

    assert(sz > 1);

    // Bracket the target percentile
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].pct <= target_percentile && ps[i].pct >= target_percentile) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket, then it wasn't available.
    if (left == 0 && right == 0)
        return NAN;

    double left_pct = ps[left].pct;
    double right_pct = ps[right].pct;
    double left_val = ps[left].val;
    double right_val = ps[right].val;

    double rise = right_val - left_val;
    double run = right_pct - left_pct;
    assert(run > 0.0);
    double slope = rise / run;

    return slope * (target_percentile - left_pct) + left_val;
}

void
cumulative_dist_free(void *void_cdf)
{
    struct CumulativeDistribution *cdf = void_cdf;
    free(cdf->percentiles);
    free(cdf);
}

/*-------------------------------------------------------------------------------------------------
 *                                 ProbabilityDistribution implementations.
 *-----------------------------------------------------------------------------------------------*/
struct PDFPoint {
    double bin_min_edge;
    double bin_max_edge;
    double bin_val;
};

static double
pdfpoint_weight(struct PDFPoint p)
{
    return p.bin_val;
}

static double
pdfpoint_center(struct PDFPoint p)
{
    if (p.bin_min_edge == 0.0) {
        return 0.0;
    }

    return (p.bin_max_edge + p.bin_min_edge) / 2.0;
}

static int
pdfpoint_compare_decreasing_weight_order(void const *a, void const *b)
{
    struct PDFPoint const *pa = a;
    struct PDFPoint const *pb = b;

    double weight_a = pdfpoint_weight(*pa);
    double weight_b = pdfpoint_weight(*pb);

    if (weight_a > weight_b)
        return -1;
    if (weight_a < weight_b)
        return 1;
    return 0;
}

#define MAX_NUM_MODES 5
struct ProbabilityDistribution {
    int size;
    struct PDFPoint *pnts;
    struct PDFPoint modes[MAX_NUM_MODES];
    int num_modes;
};

static struct ProbabilityDistribution *
probability_dist_new(int capacity)
{
    struct ProbabilityDistribution *pdf = calloc(1, sizeof(struct ProbabilityDistribution));
    assert(pdf);

    struct PDFPoint *pnts = calloc(capacity, sizeof(struct PDFPoint));
    assert(pnts);

    *pdf = (struct ProbabilityDistribution){.size = capacity, .pnts = pnts, .num_modes = 0};
    for (size_t i = 0; i < MAX_NUM_MODES; i++) {
        pdf->modes[i] =
            (struct PDFPoint){.bin_min_edge = -1.0, .bin_max_edge = 1.0, .bin_val = 0.0};
    }

    return pdf;
}

static struct PDFPoint
pdfpoint_from_percentiles(struct Percentile left, struct Percentile right)
{
    double rise = right.pct - left.pct;
    assert(rise > 0.0);
    double width = right.val - left.val;

    if (width > 0.0) {
        return (struct PDFPoint){
            .bin_min_edge = left.val, .bin_max_edge = right.val, .bin_val = rise / width};
    } else {
        return (struct PDFPoint){
            .bin_min_edge = left.val, .bin_max_edge = right.val, .bin_val = 0.0};
    }
}

static void
probability_dist_smooth(struct ProbabilityDistribution *pdf, double smooth_radius)
{
    struct PDFPoint *new = calloc(pdf->size, sizeof(struct PDFPoint));
    assert(new);

    // Smooth with a Gaussian Kernel Smoother
    double radius = smooth_radius;
    for (size_t i = 0; i < pdf->size; i++) {
        double center = pdfpoint_center(pdf->pnts[i]);
        double numerator = 0.0;
        double denom = 0.0;
        for (size_t j = 0; j < pdf->size; j++) {
            double other_center = pdfpoint_center(pdf->pnts[j]);
            double k = exp(-(pow(center - other_center, 2.0)) / (2.0 * radius * radius));
            numerator += pdfpoint_weight(pdf->pnts[j]) * k;
            denom += k;
        }

        new[i] = pdf->pnts[i];
        if (denom > 0.0) {
            new[i].bin_val = numerator / denom;
        } else {
            new[i].bin_val = 0.0;
        }
    }

    struct PDFPoint *old = pdf->pnts;
    pdf->pnts = new;
    free(old);
}

static void
probability_dist_normalize(struct ProbabilityDistribution *pdf)
{
    double max_weight = 0.0;
    for (size_t i = 0; i < pdf->size; i++) {
        double weight = pdfpoint_weight(pdf->pnts[i]);
        if (weight > max_weight) {
            max_weight = weight;
        }
    }

    for (size_t i = 0; i < pdf->size; i++) {
        pdf->pnts[i].bin_val /= max_weight;
    }
}

static void
probability_dist_fill_in_dist(struct ProbabilityDistribution *pdf,
                              struct CumulativeDistribution *cdf, double abs_min, double abs_max,
                              double smooth_radius)
{
    // Handle the left edge first.
    pdf->pnts[0] = pdfpoint_from_percentiles((struct Percentile){.pct = 0.0, .val = abs_min},
                                             cdf->percentiles[0]);

    // Handle the right edge next
    int pdf_idx = pdf->size - 1;
    pdf->pnts[pdf_idx] = pdfpoint_from_percentiles(
        cdf->percentiles[pdf_idx - 1], (struct Percentile){.pct = 100.0, .val = abs_max});

    // Loop through to get the rest.
    for (size_t i = 1; i < pdf->size - 1; i++) {
        struct Percentile left = cdf->percentiles[i - 1];
        struct Percentile right = cdf->percentiles[i];
        pdf->pnts[i] = pdfpoint_from_percentiles(left, right);
    }

    probability_dist_smooth(pdf, smooth_radius);
    probability_dist_normalize(pdf);
}

static void
probability_dist_fill_in_modes(struct ProbabilityDistribution *pdf)
{
    double weight0 = pdfpoint_weight(pdf->pnts[0]);
    bool uptrend = true;
    for (size_t i = 1; i < pdf->size; i++) {
        double weight1 = pdfpoint_weight(pdf->pnts[i]);

        if (weight1 < weight0 && uptrend) {
            if (pdf->num_modes < MAX_NUM_MODES) {
                pdf->modes[pdf->num_modes] = pdf->pnts[i - 1];
                pdf->num_modes++;
                qsort(pdf->modes, pdf->num_modes, sizeof(struct PDFPoint),
                      pdfpoint_compare_decreasing_weight_order);
            } else {
                if (weight1 >= pdfpoint_weight(pdf->modes[MAX_NUM_MODES - 1])) {
                    pdf->modes[MAX_NUM_MODES - 1] = pdf->pnts[i - 1];
                    qsort(pdf->modes, pdf->num_modes, sizeof(struct PDFPoint),
                          pdfpoint_compare_decreasing_weight_order);
                }
            }
        }

        if (weight1 >= weight0) {
            uptrend = true;
        } else {
            uptrend = false;
        }

        weight0 = weight1;
    }
}

struct ProbabilityDistribution *
probability_dist_calc(struct CumulativeDistribution *cdf, double abs_min, double abs_max,
                      double smooth_radius)
{
    assert(cdf);
    assert(cdf->size > 4);
    assert(abs_min < abs_max);

    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }

    assert(abs_min <= cdf->percentiles[0].val);
    assert(abs_max > cdf->percentiles[cdf->size - 1].val);

    struct ProbabilityDistribution *pdf = probability_dist_new(cdf->size + 1);

    probability_dist_fill_in_dist(pdf, cdf, abs_min, abs_max, smooth_radius);
    probability_dist_fill_in_modes(pdf);

    return pdf;
}

int
probability_dist_num_modes(struct ProbabilityDistribution *pdf)
{
    return pdf->num_modes;
}

double
probability_dist_get_mode(struct ProbabilityDistribution *pdf, int mode_num)
{
    assert(mode_num <= MAX_NUM_MODES);

    if (mode_num > pdf->num_modes) {
        return NAN;
    }

    return pdfpoint_center(pdf->modes[mode_num - 1]);
}

double
probability_dist_get_mode_weight(struct ProbabilityDistribution *pdf, int mode_num)
{
    assert(mode_num <= MAX_NUM_MODES);

    if (mode_num > pdf->num_modes) {
        return NAN;
    }

    return pdfpoint_weight(pdf->modes[mode_num - 1]);
}

void
probability_dist_free(void *ptr)
{
    struct ProbabilityDistribution *pdf = ptr;

    free(pdf->pnts);
    free(pdf);
}
