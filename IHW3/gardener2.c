#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "runner.h"

static volatile sig_atomic_t stop_flag = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_flag = 1;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
                "Использование: %s move_time_us process_time_us\n", argv[0]);
        return 1;
    }

    useconds_t move_time_us  = (useconds_t)strtoul(argv[1], NULL, 10);
    useconds_t proc_time_us  = (useconds_t)strtoul(argv[2], NULL, 10);

    shared_data_t *sh = attach_shared(/*create=*/0);
    if (!sh) {
        fprintf(stderr, "gardener2: не удалось подключиться к shared memory\n");
        return 1;
    }

    sem_t *mutex = open_mutex(/*create=*/0);
    if (!mutex) {
        detach_shared(sh);
        return 1;
    }

    printf("gardener2 (pid=%d) стартовал\n", getpid());

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    run_gardener(2, sh, mutex, move_time_us, proc_time_us);

    printf("gardener2 (pid=%d) завершил работу\n", getpid());

    close_mutex(mutex);
    detach_shared(sh);
    return 0;
}
