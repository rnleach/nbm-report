#include "table.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct Column {
    char *col_label;
    char *col_format;
    enum ColumnType col_type;
    int col_width;
    char **text_values;
    bool double_left_border;
    double *values1;
    double *values2;
    bool blank_zeros;
};

static double *
column_new_double_vec(int num_rows)
{
    double *vec = calloc(num_rows, sizeof(double));
    assert(vec);
    for (int i = 0; i < num_rows; i++) {
        vec[i] = NAN;
    }
    return vec;
}

static void
column_init(struct Column ptr[static 1], enum ColumnType type, int num_rows, int width,
            char const *col_label, char const *col_fmt)
{
    ptr->col_type = type;

    switch (type) {
    case Table_ColumnType_TEXT:
        ptr->text_values = calloc(num_rows, sizeof(char *));
        assert(ptr->text_values);
        break;
    case Table_ColumnType_AVG_STDEV:
        ptr->values2 = column_new_double_vec(num_rows);
        // Intentional Fall through.
    case Table_ColumnType_VALUE:
        ptr->values1 = column_new_double_vec(num_rows);
        break;
    default:
        Stopif(true, exit(EXIT_FAILURE), "Invalid ColumnType: %d", type);
    }

    ptr->col_width = width;
    ptr->double_left_border = false;

    size_t label_len = strlen(col_label) + 1;
    char *label_buf = calloc(label_len, sizeof(char));
    assert(label_buf);
    strncpy(label_buf, col_label, label_len);
    ptr->col_label = label_buf;

    size_t fmt_len = strlen(col_fmt) + 1;
    char *fmt_buf = calloc(fmt_len, sizeof(char));
    assert(fmt_buf);
    strncpy(fmt_buf, col_fmt, fmt_len);
    ptr->col_format = fmt_buf;

    ptr->blank_zeros = false;
}

static void
column_free(struct Column *ptr, int num_rows)
{
    free(ptr->col_label);
    free(ptr->col_format);

    if (ptr->text_values) {
        for (int i = 0; i < num_rows; i++) {
            free(ptr->text_values[i]);
        }
    }

    free(ptr->text_values);
    free(ptr->values1);
    free(ptr->values2);
}

struct Table {
    int num_cols;
    int num_rows;
    char *title;
    struct Column *cols;
    bool *printable;
};

struct Table *
table_new(int num_cols, int num_rows)
{
    struct Table *new = calloc(1, sizeof(struct Table));
    assert(new);

    struct Column *new_cols = calloc(num_cols, sizeof(struct Column));
    assert(new_cols);

    bool *flags = calloc(num_rows, sizeof(bool));
    assert(flags);
    for (int i = 0; i < num_rows; i++) {
        flags[i] = false;
    }

    *new = (struct Table){
        .num_cols = num_cols,
        .num_rows = num_rows,
        .title = 0,
        .cols = new_cols,
        .printable = flags,
    };

    return new;
}

void
table_free(struct Table **ptrptr)
{
    struct Table *ptr = *ptrptr;

    free(ptr->title);
    for (int i = 0; i < ptr->num_cols; i++) {
        column_free(&ptr->cols[i], ptr->num_rows);
    }
    free(ptr->printable);
    free(ptr);

    *ptrptr = 0;
}

void
table_add_title(struct Table *tbl, int str_len, char const title[str_len + 1])
{
    tbl->title = calloc(str_len + 1, sizeof(char));
    assert(tbl->title);

    strncpy(tbl->title, title, str_len + 1);
}

void
table_add_column(struct Table *tbl, int col_num, enum ColumnType type, char const *col_label,
                 char const *col_fmt, int col_width)
{
    assert(col_label);
    assert(col_fmt);
    assert(tbl->num_cols > col_num);
    assert(col_num >= 0);

    struct Column *col = &tbl->cols[col_num];

    // Check that this hasn't been set up already.
    assert(!col->col_label);
    assert(!col->col_format);

    column_init(col, type, tbl->num_rows, col_width, col_label, col_fmt);
}

void
table_set_double_left_border(struct Table *tbl, int col_num)
{
    assert(tbl->num_cols > col_num);
    assert(col_num >= 0);

    tbl->cols[col_num].double_left_border = true;
}

void
table_set_blank_zeros(struct Table *tbl, int col_num)
{
    assert(tbl->num_cols > col_num);
    assert(col_num >= 0);

    tbl->cols[col_num].blank_zeros = true;
}

void
table_set_string_value(struct Table *tbl, int col_num, int row_num, int str_len,
                       char const value[str_len + 1])
{
    assert(tbl->num_rows > row_num);
    assert(row_num >= 0);

    struct Column *col = &tbl->cols[col_num];
    Stopif(col->col_type != Table_ColumnType_TEXT, exit(EXIT_FAILURE),
           "Can't assign text to column.");

    char *buf = calloc(str_len + 1, sizeof(char));
    assert(buf);
    strncpy(buf, value, str_len + 1);

    col->text_values[row_num] = buf;

    tbl->printable[row_num] = true;
}

void
table_set_value(struct Table *tbl, int col_num, int row_num, double value)
{
    assert(tbl->num_cols > col_num);
    assert(tbl->num_rows > row_num);
    assert(col_num >= 0 && row_num >= 0);

    struct Column *col = &tbl->cols[col_num];

    Stopif(col->col_type != Table_ColumnType_VALUE, exit(EXIT_FAILURE),
           "Can't assign single value to column.");

    col->values1[row_num] = value;

    tbl->printable[row_num] = true;
}

void
table_set_avg_std(struct Table *tbl, int col_num, int row_num, double avg, double stdev)
{
    assert(tbl->num_cols > col_num);
    assert(tbl->num_rows > row_num);
    assert(col_num >= 0 && row_num >= 0);

    struct Column *col = &tbl->cols[col_num];

    Stopif(col->col_type != Table_ColumnType_AVG_STDEV, exit(EXIT_FAILURE),
           "Can't assign average and standard deviation to column.");

    col->values1[row_num] = avg;
    col->values2[row_num] = stdev;

    tbl->printable[row_num] = true;
}

static int
calc_table_width(struct Table *tbl)
{
    // The vertical bar on the left is one character wide.
    int width = 1;

    // Sum the width for each column.
    for (int c = 0; c < tbl->num_cols; c++) {
        width += tbl->cols[c].col_width;
        width += 1; // a vertical bar to the right of the column
    }

    return width;
}

static void
print_centered(char const *str, int width, FILE *out)
{

    int left_padding = 0;
    int right_padding = 0;
    if (str) {
        right_padding = (width - strlen(str)) / 2;
        left_padding = (width - strlen(str)) - right_padding;
    } else {
        left_padding = width;
        right_padding = 0;
    }

    for (int i = 0; i < left_padding; i++) {
        fputc(' ', out);
    }
    fprintf(out, "%s", str);
    for (int i = 0; i < right_padding; i++) {
        fputc(' ', out);
    }
}

static void
print_header(struct Table *tbl, FILE *out)
{
    int table_width = calc_table_width(tbl);

    // Top bar.
    fprintf(out, "┌");
    for (int i = 0; i < table_width - 2; i++) {
        fprintf(out, "─");
    }
    fprintf(out, "┐\n");

    // Print the title, centered.
    fprintf(out, "│");
    print_centered(tbl->title, table_width - 2, out);
    fprintf(out, "│\n");

    // Print the top border.
    fprintf(out, "├");
    for (int i = 0; i < tbl->cols[0].col_width; i++) {
        fprintf(out, "─");
    }
    for (int col = 1; col < tbl->num_cols; col++) {
        if (tbl->cols[col].double_left_border) {
            fprintf(out, "╥");
        } else {
            fprintf(out, "┬");
        }
        for (int i = 0; i < tbl->cols[col].col_width; i++) {
            fprintf(out, "─");
        }
    }
    fprintf(out, "┤\n");

    // Print the column labels
    for (int col = 0; col < tbl->num_cols; col++) {
        if (tbl->cols[col].double_left_border) {
            fprintf(out, "║");
        } else {
            fprintf(out, "│");
        }
        print_centered(tbl->cols[col].col_label, tbl->cols[col].col_width, out);
    }
    fprintf(out, "│\n");

    // Print the double line below.
    fprintf(out, "╞");
    for (int i = 0; i < tbl->cols[0].col_width; i++) {
        fprintf(out, "═");
    }
    for (int col = 1; col < tbl->num_cols; col++) {
        if (tbl->cols[col].double_left_border) {
            fprintf(out, "╬");
        } else {
            fprintf(out, "╪");
        }
        for (int i = 0; i < tbl->cols[col].col_width; i++) {
            fprintf(out, "═");
        }
    }
    fprintf(out, "╡\n");
}

static char *
print_table_value(struct Table *tbl, int col_num, int row_num, int buf_size, char buf[buf_size])
{
    char *next = &buf[strlen(buf)];
    struct Column *col = &tbl->cols[col_num];
    char *fmt = col->col_format;

    if (tbl->cols[col_num].double_left_border) {
        sprintf(next, "║");
    } else {
        sprintf(next, "│");
    }
    next = &next[strlen(next)];

    char small_buf[64] = {0};
    switch (col->col_type) {
    case Table_ColumnType_TEXT: {
        char *val = col->text_values[row_num];
        if (val) {
            sprintf(small_buf, fmt, val);
            sprintf(next, "%-*s", col->col_width, small_buf);
        } else {
            // Fill with spaces.
            for (int i = 0; i < col->col_width; i++) {
                next[i] = ' ';
            }
        }
        break;
    }
    case Table_ColumnType_VALUE: {
        double val = col->values1[row_num];

        if (val == 0.0 && col->blank_zeros) {
            for (int i = 0; i < col->col_width; i++) {
                next[i] = ' ';
            }
            next[col->col_width] = '\0';
        } else if (isnan(val)) {
            for (int i = 0; i < col->col_width; i++) {
                if (i == col->col_width - 2)
                    next[i] = '-';
                else
                    next[i] = ' ';
            }
            next[col->col_width] = '\0';
        } else {
            sprintf(small_buf, fmt, val);
            sprintf(next, "%*s", col->col_width, small_buf);
        }
        break;
    }
    case Table_ColumnType_AVG_STDEV: {
        double avg = col->values1[row_num];
        double stdev = col->values2[row_num];

        sprintf(small_buf, fmt, avg, stdev);
        sprintf(next, "%*s", col->col_width, small_buf);
        break;
    }
    default:
        assert(false);
    }

    next = &next[strlen(next)];
    return next;
}

static void
print_rows(struct Table *tbl, FILE *out)
{
    static char buf[512] = {0};

    for (int row = 0; row < tbl->num_rows; row++) {
        if (!tbl->printable[row]) {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        char *next = &buf[0];

        for (int col = 0; col < tbl->num_cols; col++) {
            next = print_table_value(tbl, col, row, sizeof(buf) - (next - buf), next);
        }

        sprintf(next, "│\n");

        fputs(buf, out);
    }
}

static void
print_bottom(struct Table *tbl, FILE *out)
{
    fprintf(out, "╘");
    for (int i = 0; i < tbl->cols[0].col_width; i++) {
        fprintf(out, "═");
    }
    for (int col = 1; col < tbl->num_cols; col++) {
        if (tbl->cols[col].double_left_border) {
            fprintf(out, "╩");
        } else {
            fprintf(out, "╧");
        }
        for (int i = 0; i < tbl->cols[col].col_width; i++) {
            fprintf(out, "═");
        }
    }
    fprintf(out, "╛\n");
}

void
table_display(struct Table *tbl, FILE *out)
{
    print_header(tbl, out);
    print_rows(tbl, out);
    print_bottom(tbl, out);
}
