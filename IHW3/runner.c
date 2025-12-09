#define _XOPEN_SOURCE 700

#include "runner.h"
#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>

void print_garden_safe(shared_data_t *sh, sem_t *mutex)
{
    if (!sh || !mutex) return;

    sem_wait(mutex);
    printf("\033[H\033[J");
    printf("Обработано %d из %d\n",
           sh->garden.done_cells, sh->garden.total_work_cells);
    garden_print(&sh->garden);
    sem_post(mutex);
}

static void send_log(mqd_t mq, int id, int row, int col, char action)
{
    if (mq == (mqd_t)-1) return;

    log_msg_t msg;
    msg.gardener_id = id;
    msg.row = row;
    msg.col = col;
    msg.action = action;

    if (mq_send(mq, (const char *)&msg, sizeof(msg), 0) == -1) {
        if (errno != EAGAIN) {
            perror("mq_send");
        }
    }
}

void run_gardener(int id,
                  shared_data_t *sh,
                  sem_t *mutex,
                  useconds_t move_time_us,
                  useconds_t process_time_us)
{
    if (!sh || !mutex) return;

    garden_t *g = &sh->garden;

    int row, col, dir;

    int prev1_row = -1, prev1_col = -1, prev1_dir = 0;
    int prev2_row = -1, prev2_col = -1, prev2_dir = 0;

    int wait_iters = 0;

    unsigned long long threshold_us = 2ULL * (unsigned long long)process_time_us;
    int max_wait_iters = (move_time_us > 0)
                         ? (int)(threshold_us / move_time_us + 1)
                         : 10;

    mqd_t mq = open_log_queue(0);
    if (mq == (mqd_t)-1) {
        fprintf(stderr,
                "gardener%d: не удалось открыть очередь логов, работаю без логов\n",
                id);
    }


    sem_wait(mutex);
    if (id == 1) {
        gardener1_start(g, &row, &col, &dir);
    } else {
        gardener2_start(g, &row, &col, &dir);
    }

    g->cells[row][col].occupied = id;
    if (id == 1) {
        g->g1_row = row;
        g->g1_col = col;
    } else {
        g->g2_row = row;
        g->g2_col = col;
    }
    sem_post(mutex);

    send_log(mq, id, row, col, 'S');


    for (;;) {
        sem_wait(mutex);

        if (g->stop || g->done_cells >= g->total_work_cells) {
            g->stop = 1;
            sem_post(mutex);
            break;
        }

        cell_t *cell = &g->cells[row][col];

        if (id == 1) {
            g->g1_row = row;
            g->g1_col = col;
        } else {
            g->g2_row = row;
            g->g2_col = col;
        }

        int need_process = 0;

        if (cell->type == CELL_EMPTY) {
            cell->type = (id == 1) ? CELL_DONE_G1 : CELL_DONE_G2;
            g->done_cells++;
            need_process = 1;
        }

        sem_post(mutex);

        usleep(move_time_us);
        if (need_process) {
            usleep(process_time_us);
            send_log(mq, id, row, col, 'P');
        }

        while (1) {
            sem_wait(mutex);

            if (g->stop || g->done_cells >= g->total_work_cells) {
                g->stop = 1;
                sem_post(mutex);
                goto finish;
            }

            int nr = row;
            int nc = col;
            int nd = dir;

            if (id == 1)
                gardener1_next(g, &nr, &nc, &nd);
            else
                gardener2_next(g, &nr, &nc, &nd);

            if (nr == row && nc == col) {
                g->stop = 1;
                sem_post(mutex);
                goto finish;
            }

            cell_t *next = &g->cells[nr][nc];

            if (next->occupied != 0 && next->occupied != id) {
                wait_iters++;

                if (id == 1 && wait_iters >= max_wait_iters && prev2_row != -1) {
                    g->cells[row][col].occupied = 0;

                    row = prev2_row;
                    col = prev2_col;
                    dir = prev2_dir;

                    g->cells[row][col].occupied = id;
                    g->g1_row = row;
                    g->g1_col = col;

                    printf("gardener%d: возможный дедлок, откат к (%d,%d)\n",
                           id, row, col);

                    prev1_row = prev2_row = -1;
                    prev1_col = prev2_col = -1;
                    prev1_dir = prev2_dir = 0;
                    wait_iters = 0;

                    sem_post(mutex);
                    send_log(mq, id, row, col, 'M');
                    break;
                }

                sem_post(mutex);
                usleep(move_time_us);
                continue;
            }

            prev2_row = prev1_row;
            prev2_col = prev1_col;
            prev2_dir = prev1_dir;

            prev1_row = row;
            prev1_col = col;
            prev1_dir = dir;

            g->cells[row][col].occupied = 0;

            row = nr;
            col = nc;
            dir = nd;

            next->occupied = id;

            if (id == 1) {
                g->g1_row = row;
                g->g1_col = col;
            } else {
                g->g2_row = row;
                g->g2_col = col;
            }

            wait_iters = 0;

            sem_post(mutex);
            send_log(mq, id, row, col, 'M');
            break;
        }
    }

finish:
    sem_wait(mutex);
    if (row >= 0 && row < g->M && col >= 0 && col < g->N) {
        g->cells[row][col].occupied = 0;
    }
    if (id == 1) {
        g->g1_row = -1;
        g->g1_col = -1;
    } else {
        g->g2_row = -1;
        g->g2_col = -1;
    }
    printf("Садовник %d завершил работу (pid=%d)\n", id, getpid());
    sem_post(mutex);

    send_log(mq, id, row, col, 'F');
    close_log_queue(mq);
}
