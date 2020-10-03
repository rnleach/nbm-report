#pragma once

#include <stdio.h>

/** A table meant for printing to the terminal. */
struct Table;

/** Allocate and create a new table.
 *
 * The table is created and the row and column labels are initialized to null strings. They will
 * print as empty cells if a value is never set for them. The actual cells in the table are all
 * double values and are initialized to NaN. When the table is printed, any remaining NaN values
 * are printed as a single, centered dash, ' - '.
 *
 * \param num_cols is the number of columns not including the row labels.
 * \param num_rows is the number of rows not including the column lables.
 **/
struct Table *table_new(int num_cols, int num_rows);

/** Destroy and free a table.
 *
 * The passed in pointer is zeroed.
 */
void table_free(struct Table **);

/** Add a title to the table.
 *
 * The title string is copied into the table.
 */
void table_add_title(struct Table *tbl, int str_len, char const title[str_len + 1]);

/** Add a column.
 *
 * The title and format strings are copied into the table.
 *
 * \param tbl is the target table to add a column to.
 * \param col_num is the column number to set the title for. If \c col_num is <0, then it sets the
 * title for the 'row label' column. Otherwise column numbers start at 0. If \c col_num is out of
 * range for the arguments passed in \c table_new(...), then the whole program aborts if assertions
 * are enabled. If they are not, it is not checked and the behavior undefined.
 * \param str_len is the length of the \c col_title string.
 * \param col_title is the string to copy in.
 * \param fmt_len is the length of the \c col_fmt string.
 * \param col_fmt is a \c printf format string which will be used to format the \c double values in
 * that column. The format specifier should contain only '%lf' format that specifies the precision
 * to use in the format string.
 * \param col_width is the width of the column in printable characters. If outputs, including
 * column labels are longer than this, they will be truncated.
 */
void table_add_column(struct Table *tbl, int col_num, int str_len,
                      char const col_title[str_len + 1], int fmt_len, char const *col_fmt,
                      int col_width);

/** Add a row label.
 *
 * The label string is copied into the table.
 *
 * \param tbl is the target table to add a column to.
 * \param row_num is the row number to set the lable for. Row numbers start at 0. If \c row_num is
 * out of range for the arguments passed in \c table_new(...), then the whole program aborts if
 * assertions are enabled. If they are not, it is not checked and the behavior is undefined.
 * \param str_len is the length of the \c col_title string.
 * \param col_title is the string to copy in.
 */
void table_add_row_label(struct Table *tbl, int row_num, int str_len,
                         char const row_lable[str_len + 1]);

/** Add a value to the table.
 *
 * All values in the table default to NaN, which is printed as a ' - ' string. This overwrites the
 * value at \c col_num, \c row_num.
 */
void table_add_value(struct Table *tbl, int col_num, int row_num, double value);

/** Display the table. */
void table_display(struct Table *tbl, FILE *out);
