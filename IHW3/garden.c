#include "garden.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

void garden_init(garden_t *g,
                 int M, int N,
                 int min_block_pct,
                 int max_block_pct)
{
    if (!g) return;

    M = clamp(M, 1, MAX_M);
    N = clamp(N, 1, MAX_N);

    g->M = M;
    g->N = N;
    g->stop = 0;
    g->done_cells = 0;
    g->total_work_cells = 0;

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            g->cells[i][j].type = CELL_EMPTY;
            g->cells[i][j].occupied = 0;
        }
    }

    if (min_block_pct < 0) min_block_pct = 0;
    if (max_block_pct > 90) max_block_pct = 90;
    if (max_block_pct < min_block_pct) max_block_pct = min_block_pct;

    int block_pct = min_block_pct;
    if (max_block_pct > min_block_pct) {
        block_pct += rand() % (max_block_pct - min_block_pct + 1);
    }

    int total_cells = M * N;
    int to_block = total_cells * block_pct / 100;

    int blocked = 0;
    while (blocked < to_block) {
        int r = rand() % M;
        int c = rand() % N;
        if (g->cells[r][c].type == CELL_EMPTY) {
            g->cells[r][c].type = CELL_BLOCKED;
            blocked++;
        }
    }

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (g->cells[i][j].type != CELL_BLOCKED) {
                g->total_work_cells++;
            }
        }
    }
    
    {
        int r = 0;
        int c = 0;
        int dir = +1;

        g->g1_row = 0;
        g->g1_col = 0;

        for (int step = 0; step < M * N; ++step) {
            if (g->cells[r][c].type != CELL_BLOCKED) {
                g->g1_row = r;
                g->g1_col = c;
                break;
            }
            gardener1_next(g, &r, &c, &dir);
        }
    }


    {
        int r = M - 1;
        int c = N - 1;
        int dir = -1;

        g->g2_row = M - 1;
        g->g2_col = N - 1;

        for (int step = 0; step < M * N; ++step) {
            if (g->cells[r][c].type != CELL_BLOCKED) {
                g->g2_row = r;
                g->g2_col = c;
                break;
            }
            gardener2_next(g, &r, &c, &dir);
        }
    }

    g->cells[g->g1_row][g->g1_col].occupied = 1;
    g->cells[g->g2_row][g->g2_col].occupied = 2;
}

void garden_print(const garden_t *g)
{
    if (!g) return;

    int M = g->M;
    int N = g->N;

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            char ch;

            int is_g1 = (i == g->g1_row && j == g->g1_col);
            int is_g2 = (i == g->g2_row && j == g->g2_col);

            if (is_g1 && is_g2) {
                ch = 'X';
            } else if (is_g1) {
                ch = 'A';
            } else if (is_g2) {
                ch = 'B';
            } else {
                switch (g->cells[i][j].type) {
                    case CELL_BLOCKED: ch = '#'; break;
                    case CELL_EMPTY:   ch = '.'; break;
                    case CELL_DONE_G1: ch = '1'; break;
                    case CELL_DONE_G2: ch = '2'; break;
                    default:           ch = '?'; break;
                }
            }

            putchar(ch);
            putchar(' ');
        }
        putchar('\n');
    }
    putchar('\n');
}


void gardener1_start(const garden_t *g, int *row, int *col, int *dir)
{
    if (!g || !row || !col || !dir) return;
    *row = g->g1_row;
    *col = g->g1_col;
    *dir = +1;
}

void gardener1_next(const garden_t *g, int *row, int *col, int *dir)
{
    if (!g || !row || !col || !dir) return;

    int M = g->M;
    int N = g->N;

    if (*dir == +1) {
        if (*col < N - 1) {
            (*col)++;
        } else {
            if (*row < M - 1) {
                (*row)++;
                *dir = -1;
            }
        }
    } else {
        if (*col > 0) {
            (*col)--;
        } else {
            if (*row < M - 1) {
                (*row)++;
                *dir = +1;
            }
        }
    }
}


void gardener2_start(const garden_t *g, int *row, int *col, int *dir)
{
    if (!g || !row || !col || !dir) return;
    *row = g->g2_row;
    *col = g->g2_col;
    *dir = -1;
}

void gardener2_next(const garden_t *g, int *row, int *col, int *dir)
{
    if (!g || !row || !col || !dir) return;

    int M = g->M;
    int N = g->N;

    if (*dir == -1) {

        if (*row > 0) {
            (*row)--;
        } else {
            if (*col > 0) {
                (*col)--;
                *dir = +1;
            }
        }
    } else {
        if (*row < M - 1) {
            (*row)++;
        } else {
            if (*col > 0) {
                (*col)--;
                *dir = -1;
            }
        }
    }
}
