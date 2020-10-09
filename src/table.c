#include "table.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct Table {
    int num_cols;
    int num_rows;
    char *title;
    char **col_labels;
    char **col_formats;
    int *col_widths;
    char *left_col_label; // Column Label for the column of row labels, the leftmost column.
    int left_col_width;
    char **row_labels;
    double *vals;
};

struct Table *
table_new(int num_cols, int num_rows)
{
    struct Table *new = calloc(1, sizeof(struct Table));
    assert(new);

    char **buf = calloc(2 * num_cols + num_rows, sizeof(char *));
    assert(buf);

    *new = (struct Table){
        .num_cols = num_cols,
        .num_rows = num_rows,
        .title = 0,
        .col_labels = &buf[0],
        .col_formats = &buf[num_cols],
        .col_widths = calloc(num_cols, sizeof(int)),
        .left_col_label = 0,
        .row_labels = &buf[2 * num_cols],
        .vals = calloc(num_cols * num_rows, sizeof(double)),
    };

    assert(new->col_labels);
    assert(new->col_formats);
    assert(new->col_widths);
    assert(new->row_labels);
    assert(new->vals);

    for (int i = 0; i < num_cols * num_rows; i++) {
        new->vals[i] = NAN;
    }

    return new;
}

void
table_free(struct Table **ptrptr)
{
    struct Table *ptr = *ptrptr;

    free(ptr->title);
    for (int i = 0; i < ptr->num_cols; i++) {
        free(ptr->col_labels[i]);
        free(ptr->col_formats[i]);
    }
    for (int i = 0; i < ptr->num_rows; i++) {
        free(ptr->row_labels[i]);
    }
    free(ptr->col_labels);
    free(ptr->col_widths);
    free(ptr->left_col_label);
    free(ptr->vals);
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
table_add_column(struct Table *tbl, int col_num, int str_len, char const col_label[str_len + 1],
                 int fmt_len, char const col_fmt[fmt_len + 1], int col_width)
{
    assert(tbl->num_cols > col_num);
    assert(col_num >= -1);

    if (col_num >= 0) { // Not the row label column header
        char *label_buf = calloc(str_len + 1, sizeof(char));
        assert(label_buf);
        strncpy(label_buf, col_label, str_len + 1);
        tbl->col_labels[col_num] = label_buf;

        char *fmt_buf = calloc(fmt_len + 1, sizeof(char));
        assert(fmt_buf);
        strncpy(fmt_buf, col_fmt, fmt_len + 1);
        tbl->col_formats[col_num] = fmt_buf;

        tbl->col_widths[col_num] = col_width;
    } else {
        char *label_buf = calloc(str_len + 1, sizeof(char));
        assert(label_buf);
        strncpy(label_buf, col_label, str_len + 1);
        tbl->left_col_label = label_buf;

        tbl->left_col_width = col_width;
    }
}

void
table_add_row_label(struct Table *tbl, int row_num, int str_len, char const row_label[str_len + 1])
{
    assert(tbl->num_rows > row_num);
    assert(row_num >= 0);

    char *buf = calloc(str_len + 1, sizeof(char));
    assert(buf);
    strncpy(buf, row_label, str_len + 1);
    tbl->row_labels[row_num] = buf;
}

void
table_add_value(struct Table *tbl, int col_num, int row_num, double value)
{
    assert(tbl->num_cols > col_num);
    assert(tbl->num_rows > row_num);
    assert(col_num >= 0 && row_num >= 0);

    tbl->vals[tbl->num_cols * row_num + col_num] = value;
}

static int
calc_table_width(struct Table *tbl)
{
    // The vertical bar on the left is one character wide.
    int width = 1;

    // Sum the width for each column.
    for (int c = 0; c < tbl->num_cols; c++) {
        width += tbl->col_widths[c];
        width += 1; // a vertical bar to the right of the column
    }

    // Don't forget the label column
    width += tbl->left_col_width;
    width += 1; // a vertical bar to the right of the column

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
    for (int i = 0; i < tbl->left_col_width; i++) {
        fprintf(out, "─");
    }
    for (int col = 0; col < tbl->num_cols; col++) {
        fprintf(out, "┬");
        for (int i = 0; i < tbl->col_widths[col]; i++) {
            fprintf(out, "─");
        }
    }
    fprintf(out, "┤\n");

    // Print the column labels
    fprintf(out, "│");
    print_centered(tbl->left_col_label, tbl->left_col_width, out);
    for (int col = 0; col < tbl->num_cols; col++) {
        fprintf(out, "│");
        print_centered(tbl->col_labels[col], tbl->col_widths[col], out);
    }
    fprintf(out, "│\n");

    // Print the double line below.
    fprintf(out, "╞");
    for (int i = 0; i < tbl->left_col_width; i++) {
        fprintf(out, "═");
    }
    for (int col = 0; col < tbl->num_cols; col++) {
        fprintf(out, "╪");
        for (int i = 0; i < tbl->col_widths[col]; i++) {
            fprintf(out, "═");
        }
    }
    fprintf(out, "╡\n");
}

static void
print_rows(struct Table *tbl, FILE *out)
{
    static char buf[512] = {0};

    for (int row = 0; row < tbl->num_rows; row++) {
        memset(buf, 0, sizeof(buf));

        sprintf(buf, "│");
        char *next = &buf[strlen(buf)];

        int left_padding = 0;
        if (tbl->row_labels[row]) {
            left_padding = tbl->left_col_width - strlen(tbl->row_labels[row]);
        } else {
            left_padding = tbl->left_col_width;
        }
        for (int i = 0; i < left_padding; i++) {
            next[i] = ' ';
        }
        next = &buf[strlen(buf)];

        if (tbl->row_labels[row]) {
            sprintf(next, "%s", tbl->row_labels[row]);
            next = &buf[strlen(buf)];
        }

        for (int col = 0; col < tbl->num_cols; col++) {
            sprintf(next, "│");
            next = &buf[strlen(buf)];
            sprintf(next, tbl->col_formats[col], tbl->vals[tbl->num_cols * row + col]);
            next = &buf[strlen(buf)];
        }
        sprintf(next, "│\n");
        wipe_nans(buf);

        fputs(buf, out);
    }
}

static void
print_bottom(struct Table *tbl, FILE *out)
{
    fprintf(out, "╘");
    for (int i = 0; i < tbl->left_col_width; i++) {
        fprintf(out, "═");
    }
    for (int col = 0; col < tbl->num_cols; col++) {
        fprintf(out, "╧");
        for (int i = 0; i < tbl->col_widths[col]; i++) {
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

    // assert(false);
}
