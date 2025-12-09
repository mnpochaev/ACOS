#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc.h"
#include "runner.h"

int main(int argc, char *argv[])
{
    useconds_t interval_us = 200000;
    if (argc >= 2) {
        interval_us = (useconds_t)strtoul(argv[1], NULL, 10);
        if (interval_us == 0) {
            interval_us = 200000;
        }
    }

    shared_data_t *sh = attach_shared(/*create=*/0);
    if (!sh) {
        fprintf(stderr, "multi_observer: не удалось подключиться к shared memory\n");
        return 1;
    }

    sem_t *mutex = open_mutex(/*create=*/0);
    if (!mutex) {
        detach_shared(sh);
        return 1;
    }

    printf("multi_observer (pid=%d) стартовал, период обновления %u мкс\n",
           getpid(), (unsigned)interval_us);

    while (1) {
        print_garden_safe(sh, mutex);

        sem_wait(mutex);
        int done  = sh->garden.done_cells;
        int total = sh->garden.total_work_cells;
        int stop  = sh->garden.stop;
        sem_post(mutex);

        if (stop || done >= total) {
            printf("multi_observer (pid=%d): сад обработан (%d/%d), выхожу\n",
                   getpid(), done, total);
            break;
        }

        usleep(interval_us);
    }

    close_mutex(mutex);
    detach_shared(sh);
    return 0;
}
