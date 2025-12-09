#ifndef GARDEN_H
#define GARDEN_H

#include <stddef.h>

#define MAX_M 50
#define MAX_N 50

typedef enum {
    CELL_EMPTY   = 0,
    CELL_BLOCKED = 1,
    CELL_DONE_G1 = 2,
    CELL_DONE_G2 = 3
} cell_type_t;

typedef struct {
    int type;
    int occupied;
} cell_t;

typedef struct {
    int M, N;
    int total_work_cells;
    int done_cells;
    int stop;

    int g1_row, g1_col;
    int g2_row, g2_col;

    cell_t cells[MAX_M][MAX_N];
} garden_t;

void garden_init(garden_t *g,
                 int M, int N,
                 int min_block_pct,
                 int max_block_pct);

void garden_print(const garden_t *g);

void gardener1_start(const garden_t *g, int *row, int *col, int *dir);
void gardener2_start(const garden_t *g, int *row, int *col, int *dir);

void gardener1_next(const garden_t *g, int *row, int *col, int *dir);
void gardener2_next(const garden_t *g, int *row, int *col, int *dir);

#endif
