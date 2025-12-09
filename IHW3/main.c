#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#include "garden.h"

#define SHM_NAME "/garden_shm_v1"

typedef struct {
    garden_t garden;
    sem_t    mutex;
} shared_data_t;

static volatile sig_atomic_t g_stop_flag = 0;

void sigint_handler(int sig) {
    (void)sig;
    g_stop_flag = 1;
}

void print_garden_safe(shared_data_t *sh) {
    sem_wait(&sh->mutex);
    printf("\033[H\033[J");
    printf("Обработано %d из %d\n",
           sh->garden.done_cells, sh->garden.total_work_cells);
    garden_print(&sh->garden);
    sem_post(&sh->mutex);
}

void run_gardener(int id,
                  shared_data_t *sh,
                  useconds_t move_time_us,
                  useconds_t process_time_us)
{
    garden_t *g = &sh->garden;

    int row, col, dir;

    int prev1_row = -1, prev1_col = -1, prev1_dir = 0;
    int prev2_row = -1, prev2_col = -1, prev2_dir = 0;

    int wait_iters = 0;

    unsigned long long threshold_us = 2ULL * (unsigned long long)process_time_us;
    int max_wait_iters = (move_time_us > 0)
                         ? (int)(threshold_us / move_time_us + 1)
                         : 10;


    sem_wait(&sh->mutex);
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
    sem_post(&sh->mutex);


    for (;;) {

        sem_wait(&sh->mutex);

        if (g->stop || g->done_cells >= g->total_work_cells) {
            g->stop = 1;
            sem_post(&sh->mutex);
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

        sem_post(&sh->mutex);

        usleep(move_time_us);

        if (need_process) {
            usleep(process_time_us);
        }


        while (1) {
            sem_wait(&sh->mutex);

            if (g->stop || g->done_cells >= g->total_work_cells) {
                g->stop = 1;
                sem_post(&sh->mutex);
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
                sem_post(&sh->mutex);
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
                    if (id == 1) {
                    g->g1_row = row;
                    g->g1_col = col;
                    } else {
                    g->g2_row = row;
                    g->g2_col = col;
                    }

                    printf("gardener%d: возможный дедлок, откат к (%d,%d)\n",
                    id, row, col);

                    prev1_row = prev2_row = -1;
                    prev1_col = prev2_col = -1;
                    prev1_dir = prev2_dir = 0;
                    wait_iters = 0;

                    sem_post(&sh->mutex);
                    break;
                }

                sem_post(&sh->mutex);
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

            sem_post(&sh->mutex);
            break;
        }
    }

finish:
    sem_wait(&sh->mutex);
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
    printf("P1: садовник %d завершил работу (pid=%d)\n", id, getpid());
    sem_post(&sh->mutex);
}

int main(int argc, char *argv[])
{
    if (argc < 6) {
        fprintf(stderr,
                "Использование: %s M N move_time_us process_g1_us process_g2_us\n",
                argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    useconds_t move_time_us   = (useconds_t)strtoul(argv[3], NULL, 10);
    useconds_t proc1_time_us  = (useconds_t)strtoul(argv[4], NULL, 10);
    useconds_t proc2_time_us  = (useconds_t)strtoul(argv[5], NULL, 10);

    srand(time(NULL));

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    shared_data_t *sh = mmap(NULL, sizeof(shared_data_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }
    close(fd);

    if (sem_init(&sh->mutex, 1, 1) == -1) {
        perror("sem_init");
        munmap(sh, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
        return 1;
    }

    garden_init(&sh->garden, M, N, 10, 30);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return 1;
    }
    if (pid1 == 0) {
        run_gardener(1, sh, move_time_us, proc1_time_us);
        _exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return 1;
    }
    if (pid2 == 0) {
        run_gardener(2, sh, move_time_us, proc2_time_us);
        _exit(0);
    }

    while (!g_stop_flag) {
        sem_wait(&sh->mutex);
        int done  = sh->garden.done_cells;
        int total = sh->garden.total_work_cells;
        int stop  = sh->garden.stop;
        sem_post(&sh->mutex);

        print_garden_safe(sh);

        if (stop || done >= total)
            break;

        usleep(300000);
    }

    if (g_stop_flag) {
        sem_wait(&sh->mutex);
        sh->garden.stop = 1;
        sem_post(&sh->mutex);
    }

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    sem_destroy(&sh->mutex);
    munmap(sh, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);

    printf("Родитель (pid=%d) завершил работу\n", getpid());
    return 0;
}
