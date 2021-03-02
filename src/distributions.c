#include "distributions.h"
#include "utils.h"

#include <assert.h>
#include <math.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                    CDF implementations.
 *-----------------------------------------------------------------------------------------------*/
/** Internal representation of a point in a CDF. */
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

    // If the percents are equal, sort on the values.
    if (pa->val < pb->val)
        return -1;
    if (pa->val > pb->val)
        return 1;

    return 0;
}

/** Internal implementation of Cumulative distribution. */
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
    static size_t const initial_size = 13;

    struct CumulativeDistribution *new = calloc(1, sizeof(struct CumulativeDistribution));
    assert(new);

    new->percentiles = calloc(initial_size, sizeof(struct Percentile));
    assert(new->percentiles);
    new->capacity = initial_size;
    new->quantile_mapped_value = NAN;
    new->sorted = false;

    return new;
}

static bool
cumulative_dist_append_pair(struct CumulativeDistribution *dist, double percentile, double value)
{
    if (dist->capacity == dist->size) {
        size_t const new_capacity = 2 * dist->capacity;

        dist->percentiles = realloc(dist->percentiles, new_capacity * sizeof(struct Percentile));
        assert(dist->percentiles);
        dist->capacity = new_capacity;
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

GTree *
extract_exceedence_to_cdfs(GTree *tree, NBMData const *nbm, char const *col_name_format,
                           size_t num_vals, char const *const vals[num_vals], Converter convert)
{
    char col_name[256] = {0};
    for (size_t i = 0; i < num_vals; i++) {
        int num_chars = snprintf(col_name, sizeof(col_name), col_name_format, vals[i]);
        Stopif(num_chars >= sizeof col_name, return tree, "error with snprintf, buffer too small.");

        double value = convert(strtod(vals[i], 0));

        // Create an iterator, if the point is 0, no such column, so skip it and continue
        NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
        if (!it) {
            continue;
        }

        struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
        while (view.valid_time && view.value) {
            struct CumulativeDistribution *cd = g_tree_lookup(tree, view.valid_time);
            if (cd) {

                double percentile = 100.0 - *view.value;
                cumulative_dist_append_pair(cd, percentile, value);
            }

            view = nbm_data_row_iterator_next(it);
        }
        nbm_data_row_iterator_free(&it);
    }

    return tree;
}

double
cumulative_dist_pm_value(struct CumulativeDistribution const *ptr)
{
    return ptr->quantile_mapped_value;
}

static void
cumulative_dist_slide_left(struct CumulativeDistribution *ptr, size_t first_dup, size_t num_dups)
{
    size_t const offset = 1;
    struct Percentile *dest = &ptr->percentiles[first_dup + offset];
    struct Percentile *src = &ptr->percentiles[first_dup + num_dups + offset];
    size_t stride = ptr->size - (first_dup + num_dups + offset);
    memmove(dest, src, stride * sizeof(*src));
    ptr->size -= num_dups;
}

static void
cumulative_dist_cleanup(struct CumulativeDistribution *ptr)
{
    assert(ptr->sorted);

    for (size_t i = 0; i < ptr->size; i++) {
        // Anything over 100th percentile is redundant
        if (i > 1 && ptr->percentiles[i].pct >= 100.0) {
            ptr->size = i + 1;
            break;
        }

        // Force the CDF to be an increasing function. Rounding and interpolation may cause a
        // higher percentile to have a lower value when combining disparate sources. Remove the
        // points that cause the CDF to be a decreasing function anywhere.
        size_t num_bad = 0;
        size_t j = i + 1;

        while (j < ptr->size && ptr->percentiles[i].val > ptr->percentiles[j].val) {
            num_bad++;
            j++;
        }

        if (num_bad > 0) {
            cumulative_dist_slide_left(ptr, i, num_bad);
        }
    }

    // By combining disparate sources (e.g. quantile and probability of exceedance) we may end up
    // with multiple entries for the same percentile, those should be eliminated. We'll keep the
    // first one we encounter.
    for (size_t i = 0; i < ptr->size; i++) {
        size_t num_dups = 0;
        size_t j = i + 1;

        while (j < ptr->size && ptr->percentiles[i].pct == ptr->percentiles[j].pct) {
            num_dups++;
            j++;
        }

        if (num_dups > 0) {
            cumulative_dist_slide_left(ptr, i, num_dups);
        }
    }
}

static void
cumulative_dist_sort(struct CumulativeDistribution *ptr)
{
    qsort(ptr->percentiles, ptr->size, sizeof(struct Percentile), percentile_compare);
    ptr->sorted = true;
    cumulative_dist_cleanup(ptr);
}

double
interpolate_prob_of_exceedance(struct CumulativeDistribution *cdf, double target_val)
{
    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }

    struct Percentile *ps = cdf->percentiles;
    int sz = cdf->size;

    // Bracket the target value
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].val <= target_val && ps[i].val > target_val) {
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

    assert(sz > 0);

    // Bracket the target percentile
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].pct <= target_percentile && ps[i].pct > target_percentile) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket, see if we are below the lowest percentile. If that is the case, then
    // just return the lowest percentiles value. In most cases this will be 0.0 for snow and
    // precipitation. However, with temperatures this would be incorrect and NAN would be a better
    // value. Fortuneately we have a detailed temperature distribution, so this should not be an
    // issue.
    if (left == 0 && right == 0) {
        if (ps[0].pct > target_percentile) {
            return ps[0].val;
        }

        // If unable to bracket, then it wasn't available.
        return NAN;
    }

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

double
cumulative_dist_max_value(struct CumulativeDistribution *cdf)
{
    assert(cdf);

    return cdf->percentiles[cdf->size - 1].val;
}

double
cumulative_dist_min_value(struct CumulativeDistribution *cdf)
{
    assert(cdf);

    return cdf->percentiles[0].val;
}

void
cumulative_dist_free(void *void_cdf)
{
    struct CumulativeDistribution *cdf = void_cdf;
    free(cdf->percentiles);
    free(cdf);
}

void
cumulative_dist_write(struct CumulativeDistribution *cdf, FILE *f)
{
    for (size_t i = 0; i < cdf->size; i++) {
        fprintf(f, "%8lf %8lf\n", cdf->percentiles[i].val, cdf->percentiles[i].pct);
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                 ProbabilityDistribution implementations.
 *-----------------------------------------------------------------------------------------------*/
/** Internal representation of a point on a PDF. */
struct PDFPoint {
    double min;
    double max;
    double density;
};

static double
pdfpoint_density(struct PDFPoint p)
{
    return p.density;
}

static double
pdfpoint_center(struct PDFPoint p)
{
    return (p.max + p.min) / 2.0;
}

static double
pdfpoint_probability_area(struct PDFPoint p)
{
    double width = p.max - p.min;
    if (width <= 0.0) {
        return 1.0;
    }

    return width * p.density;
}

/** Internal implementation of PDF. */
struct ProbabilityDistribution {
    int size;
    struct PDFPoint *pnts;
};

static struct ProbabilityDistribution *
probability_dist_new(int capacity)
{
    struct ProbabilityDistribution *pdf = calloc(1, sizeof(struct ProbabilityDistribution));
    assert(pdf);

    struct PDFPoint *pnts = calloc(capacity, sizeof(struct PDFPoint));
    assert(pnts);

    *pdf = (struct ProbabilityDistribution){.size = capacity, .pnts = pnts};

    return pdf;
}

static struct PDFPoint
pdfpoint_from_percentiles(struct Percentile left, struct Percentile right)
{
    double rise = right.pct - left.pct;
    assert(rise >= 0.0);
    double width = right.val - left.val;
    assert(width >= 0.0);

    // Yes, may be infinity or nan
    double density = rise / width;

    return (struct PDFPoint){.min = left.val, .max = right.val, .density = density};
}

static void
probability_dist_normalize(struct ProbabilityDistribution *pdf)
{
    double total_area = 0.0;
    for (size_t i = 0; i < pdf->size; i++) {
        double area = pdfpoint_probability_area(pdf->pnts[i]);

        // We should have removed all these before getting here.
        assert(!isinf(area) && !isnan(area));

        total_area += area;
    }

    for (size_t i = 0; i < pdf->size; i++) {
        pdf->pnts[i].density /= total_area;
    }
}

static struct ProbabilityDistribution *
probability_dist_smooth(struct ProbabilityDistribution const *pdf, double smooth_radius)
{
    struct ProbabilityDistribution *new_dist = probability_dist_new(pdf->size);
    assert(new_dist);
    struct PDFPoint *new = new_dist->pnts;
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
            double density = pdfpoint_density(pdf->pnts[j]);

            // We should have removed all these before getting here.
            assert(!isinf(density) && !isnan(density));

            numerator += density * k;
            denom += k;
        }

        new[i] = pdf->pnts[i];
        if (denom > 0.0) {
            new[i].density = numerator / denom;
        } else {
            new[i].density = 0.0;
        }
    }

    probability_dist_normalize(new_dist);
    return new_dist;
}

static void
probability_dist_fill_in_dist(struct ProbabilityDistribution *pdf,
                              struct CumulativeDistribution *cdf)
{
    size_t num_pnts_added = 0;
    struct Percentile left = cdf->percentiles[0];
    struct Percentile right = cdf->percentiles[0];

    for (size_t i = 0; i < pdf->size; i++) {
        right = cdf->percentiles[i + 1];

        // Sometimes a 100th percentile can be added as derived from a probability of exceedance.
        // That can cause a false maximum and mess up the scenarios. So skip the 100th percentile.
        if (right.pct >= 100.0)
            continue;

        struct PDFPoint trial_pnt = pdfpoint_from_percentiles(left, right);

        if (!isinf(trial_pnt.density) && !isnan(trial_pnt.density)) {
            pdf->pnts[num_pnts_added] = trial_pnt;
            num_pnts_added++;
            left = right;
        }
    }

    if (num_pnts_added == 0) {
        struct PDFPoint only_pnt = pdfpoint_from_percentiles(left, left);
        only_pnt.density = 1.0;

        assert((only_pnt.max - only_pnt.min) == 0.0);
        pdf->pnts[0] = only_pnt;
        num_pnts_added = 1;
    }

    pdf->size = num_pnts_added;

    probability_dist_normalize(pdf);
}

struct ProbabilityDistribution *
probability_dist_calc(struct CumulativeDistribution *cdf)
{
    assert(cdf);

    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }

    struct ProbabilityDistribution *pdf = probability_dist_new(cdf->size - 1);

    probability_dist_fill_in_dist(pdf, cdf);

    return pdf;
}

void
probability_dist_free(void *ptr)
{
    struct ProbabilityDistribution *pdf = ptr;

    free(pdf->pnts);
    free(pdf);
}

struct ProbabilityDistribution *
probability_dist_copy(struct ProbabilityDistribution *src)
{
    struct ProbabilityDistribution *dest = probability_dist_new(src->size);
    memcpy(dest->pnts, src->pnts, src->size * sizeof(struct PDFPoint));

    return dest;
}

void
probability_dist_write(struct ProbabilityDistribution *pdf, FILE *f)
{
    for (size_t i = 0; i < pdf->size; i++) {
        fprintf(f, "%8lf %8lf\n", pdfpoint_center(pdf->pnts[i]), pdfpoint_density(pdf->pnts[i]));
    }
}
/*-------------------------------------------------------------------------------------------------
 *                                  Scenario implementations.
 *-----------------------------------------------------------------------------------------------*/
/** Internal implementation of a \ref Scenario. */
struct Scenario {
    double min;
    double max;
    double mode;
    double prob;
};

static struct Scenario *
scenario_allocate(void)
{
    struct Scenario *ptr = calloc(1, sizeof(struct Scenario));
    assert(ptr);

    ptr->mode = NAN;

    return ptr;
}

void
scenario_free(void *ptr)
{
    free(ptr);
}

double
scenario_get_mode(struct Scenario const *sc)
{
    return sc->mode;
}

double
scenario_get_minimum(struct Scenario const *sc)
{
    return sc->min;
}

double
scenario_get_maximum(struct Scenario const *sc)
{
    return sc->max;
}

double
scenario_get_probability(struct Scenario const *sc)
{
    return sc->prob;
}

static int
scenario_cmp_descending_prob(void const *a, void const *b)
{
    struct Scenario const *sa = a;
    struct Scenario const *sb = b;

    if (sa->prob > sb->prob) {
        return -1;
    }

    if (sa->prob < sb->prob) {
        return 1;
    }

    return 0;
}

#define TRENDING_UP 1
#define NO_TREND 0
#define TRENDING_DOWN -1

static GList *
find_scenarios_inner(ProbabilityDistribution const *pdf)
{
    GList *scenarios = 0;

    struct Scenario *curr = scenario_allocate();
    int trending0 = NO_TREND;
    double prob_area = 0.0;

    assert(pdf->size > 0);

    curr->min = pdf->pnts[0].min;
    if (pdf->size == 1 || pdf->pnts[1].density < pdf->pnts[0].density) {
        // We are starting at a max
        curr->mode = pdfpoint_center(pdf->pnts[0]);
        trending0 = TRENDING_DOWN;
    } else {
        trending0 = TRENDING_UP;
    }
    prob_area += pdfpoint_probability_area(pdf->pnts[0]);

    double density0 = pdfpoint_density(pdf->pnts[0]);
    for (size_t i = 1; i < pdf->size; i++) {
        double density1 = pdfpoint_density(pdf->pnts[i]);
        int trending1 = density1 < density0 ? TRENDING_DOWN : TRENDING_UP;

        if (trending0 == TRENDING_UP && trending1 == TRENDING_DOWN) {
            // At a max
            curr->mode = pdfpoint_center(pdf->pnts[i - 1]);
        } else if (trending0 == TRENDING_DOWN && trending1 == TRENDING_UP) {
            // At a min - start a new scenario
            curr->max = pdf->pnts[i].min;
            curr->prob = prob_area;
            prob_area = 0.0;

            scenarios = g_list_insert_sorted(scenarios, curr, scenario_cmp_descending_prob);
            curr = scenario_allocate();
            curr->min = pdf->pnts[i].min;
        }

        prob_area += pdfpoint_probability_area(pdf->pnts[i]);

        density0 = density1;
        trending0 = trending1;
    }

    struct PDFPoint last = pdf->pnts[pdf->size - 1];
    if (isnan(curr->mode)) {
        curr->mode = pdfpoint_center(last);
    }
    curr->max = last.max;
    curr->prob = prob_area;
    scenarios = g_list_insert_sorted(scenarios, curr, scenario_cmp_descending_prob);

    return scenarios;
}

GList *
find_scenarios(ProbabilityDistribution *pdf, double minimum_smooth_radius, double smooth_radius_inc)
{
    assert(minimum_smooth_radius > 0.0);
    assert(smooth_radius_inc > 0.0);

    GList *scenarios = 0;
    double smooth_radius = minimum_smooth_radius;
    do {
        ProbabilityDistribution *smoothed_pdf = probability_dist_smooth(pdf, smooth_radius);
        scenarios = find_scenarios_inner(smoothed_pdf);
        if (g_list_length(scenarios) <= 4) {
            // Swap the smoothed PDF into the PDF
            ProbabilityDistribution temp = {0};
            temp.size = pdf->size;
            temp.pnts = pdf->pnts;
            pdf->size = smoothed_pdf->size;
            pdf->pnts = smoothed_pdf->pnts;
            smoothed_pdf->size = temp.size;
            smoothed_pdf->pnts = temp.pnts;

            probability_dist_free(smoothed_pdf);
            break;
        }
        probability_dist_free(smoothed_pdf);
        smooth_radius += smooth_radius_inc;
    } while (true);

    return scenarios;
}

/** Used to keep track of state while mapping over a collection of PDFs to create a collection of
 * Scenarios.
 */
struct PDFToScenarioData {
    double minimum_smooth_radius;
    double smooth_radius_inc;
    GTree *scenarios;
};

static int
create_scenarios_mapping(void *key, void *val, void *user_data)
{
    time_t *valid_time = key;
    ProbabilityDistribution *pdf = val;
    struct PDFToScenarioData *data = user_data;

    GTree *scenarios = data->scenarios;

    GList *scs = find_scenarios(pdf, data->minimum_smooth_radius, data->smooth_radius_inc);

    time_t *new_key = calloc(1, sizeof(*valid_time));
    *new_key = *valid_time;

    g_tree_insert(scenarios, new_key, scs);

    return false;
}

GTree *
create_scenarios_from_pdfs(GTree *pdfs, double minimum_smooth_radius, double smooth_radius_inc)
{
    GTree *scenarios = g_tree_new_full(time_t_compare_func, 0, free, free_glist_of_scenarios);
    struct PDFToScenarioData callback_data = {.minimum_smooth_radius = minimum_smooth_radius,
                                              .smooth_radius_inc = smooth_radius_inc,
                                              .scenarios = scenarios};

    g_tree_foreach(pdfs, create_scenarios_mapping, &callback_data);

    return scenarios;
}

void
free_glist_of_scenarios(void *ptr)
{
    GList *scs = ptr;
    g_clear_list(&scs, scenario_free);
}
