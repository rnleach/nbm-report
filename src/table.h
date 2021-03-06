#pragma once

#include <stdio.h>

/** A table meant for printing to the terminal. */
typedef struct Table Table;

/** A column type descriptor. */
enum ColumnType {
    Table_ColumnType_TEXT,
    Table_ColumnType_VALUE,
    Table_ColumnType_AVG_STDEV,
    Table_ColumnType_SCENARIO,
};

/** A state type useful for use with callbacks that take a \c void \c * \c user_data argument. */
struct TableFillerState {
    int row;
    struct Table *tbl;
};

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
Table *table_new(int num_cols, int num_rows);

/** Destroy and free a table.
 *
 * The passed in pointer is zeroed.
 */
void table_free(Table **);

/** Add a title to the table.
 *
 * The title string is copied into the table.
 */
void table_add_title(Table *tbl, int str_len, char const title[str_len + 1]);

/** Add a column.
 *
 * The title and format strings are copied into the table.
 *
 * \param tbl is the target table to add a column to.
 * \param col_num is the column number. Column numbers start at 0. If \c col_num is out of range
 * for the arguments passed in \c table_new(...), then the whole program aborts if assertions are
 * enabled. If they are not, it is not checked and the behavior undefined.
 * \param type is the type of data in this column.
 * \param col_label is the string to copy in.
 * \param col_fmt is a \c printf format string which will be used to format the \c double values in
 * that column. The format specifier should contain only '%lf' format that specifies the precision
 * to use in the format string.
 * \param col_width is the width of the column in printable characters. If outputs, including
 * column labels are longer than this, they will be truncated.
 */
void table_add_column(Table *tbl, int col_num, enum ColumnType type, char const *col_label,
                      char const *col_fmt, int col_width);

/** Set the left border of a column to have a double border */
void table_set_double_left_border(Table *tbl, int col_num);

/** Set a sentinel value to be blanked out and print nothing for a column.
 *
 * This has no affect on text or avg-std columns.
 */
void table_set_blank_value(Table *tbl, int col_num, double value);

/** Add a string value to a column.
 *
 * If the column type does not match (i.e. it's not a Table_ColumnType_TEXT column), then it will
 * abort the program.
 *
 * \param tbl is the target table to add a column to.
 * \param col_num is the column number. Column numbers start at 0. If \c col_num is out of range
 * for the arguments passed in \c table_new(...), then the whole program aborts if assertions are
 * enabled. If they are not, it is not checked and the behavior undefined.
 * \param row_num is the row number of the table cell. Row numbers start at 0. If \c row_num is
 * out of range for the arguments passed in \c table_new(...), then the whole program aborts if
 * assertions are enabled. If they are not, it is not checked and the behavior is undefined.
 * \param str_len is the length of the \c value string.
 * \param value is the string to copy in.
 */
void table_set_string_value(Table *tbl, int col_num, int row_num, int str_len,
                            char const value[str_len + 1]);

/** Add a value to the table.
 *
 * If the column type does not match (i.e. it's not a Table_ColumnType_VALUE column), then it will
 * abort the program.
 *
 * All double values in the table default to NaN, which is printed as a ' - ' string. This
 * overwrites the value at \c col_num, \c row_num.
 */
void table_set_value(Table *tbl, int col_num, int row_num, double value);

/** Add an average and standard deviation value to the table.
 *
 * If the column type does not match (i.e. it's not a Table_ColumnType_AVG_STDEV column), then it
 * will abort the program.
 *
 * All double values in the table default to NaN, which is printed as a ' - ' string. This
 * overwrites the value at \c col_num, \c row_num.
 */
void table_set_avg_std(Table *tbl, int col_num, int row_num, double avg, double stdev);

/** Add a scenario value to the table.
 *
 * If the column type does not match (i.e. it's not a Table_ColumnType_SCENARIO column), then it
 * will abort the program.
 *
 * All double values in the table default to NaN, which is printed as a ' - ' or blank string. This
 * overwrites the value at \c col_num, \c row_num.
 */
void table_set_scenario(Table *tbl, int col_num, int row_num, double mode, double min_val,
                        double max_val, double prob);

/** Display the table. */
void table_display(Table *tbl, FILE *out);
