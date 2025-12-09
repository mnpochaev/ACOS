#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "ipc.h"
#include "runner.h"

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Использование: %s M N\n", argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);
    int N = atoi(argv[2]);

    srand(time(NULL));

    shared_data_t *sh = attach_shared(1);
    if (!sh) {
        fprintf(stderr, "Не удалось создать shared memory\n");
        return 1;
    }


    sem_t *mutex = open_mutex(1);
    if (!mutex) {
        detach_shared(sh);
        destroy_shared();
        return 1;
    }

    garden_init(&sh->garden, M, N, 10, 30);

    printf("Сад инициализирован: %dx%d\n", M, N);
    printf("total_work_cells = %d\n", sh->garden.total_work_cells);
    printf("Запускайте gardener1 и gardener2 в отдельных терминалах.\n");

    while (1) {
        print_garden_safe(sh, mutex);

        sem_wait(mutex);
        int done  = sh->garden.done_cells;
        int total = sh->garden.total_work_cells;
        int stop  = sh->garden.stop;
        sem_post(mutex);

        if (stop || done >= total) {
            printf("init: сад обработан (%d/%d), завершаю.\n", done, total);
            break;
        }

        usleep(50000);
    }

    close_mutex(mutex);
    detach_shared(sh);

    return 0;
}

