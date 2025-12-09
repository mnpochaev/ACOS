#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>

#include "ipc.h"
#include "runner.h"

int main(void)
{
    shared_data_t *sh = attach_shared(0);
    if (!sh) {
        fprintf(stderr, "observer: не удалось подключиться к shared memory\n");
        return 1;
    }


    sem_t *mutex = open_mutex(0);
    if (!mutex) {
        detach_shared(sh);
        return 1;
    }

    mqd_t mq = open_log_queue(1);
    if (mq == (mqd_t)-1) {
        close_mutex(mutex);
        detach_shared(sh);
        return 1;
    }

    struct mq_attr attr;
    if (mq_getattr(mq, &attr) == -1) {
        perror("mq_getattr");
        close_log_queue(mq);
        close_mutex(mutex);
        detach_shared(sh);
        return 1;
    }

    long msgsize = attr.mq_msgsize;
    char *buf = malloc(msgsize);
    if (!buf) {
        fprintf(stderr, "observer: не хватило памяти под буфер mq\n");
        close_log_queue(mq);
        close_mutex(mutex);
        detach_shared(sh);
        return 1;
    }

    printf("observer (pid=%d) стартовал\n", getpid());
    printf("Ожидаю сообщения от садовников...\n");

    while (1) {
        ssize_t n = mq_receive(mq, buf, msgsize, NULL);
        if (n == -1) {
            perror("mq_receive");
            break;
        }

        log_msg_t msg;
        if ((size_t)n >= sizeof(log_msg_t)) {
            memcpy(&msg, buf, sizeof(log_msg_t));
        } else {
            fprintf(stderr, "observer: получено слишком короткое сообщение (%zd)\n", n);
            continue;
        }

        print_garden_safe(sh, mutex);

        printf("log: gardener %d, action %c, cell (%d, %d)\n",
               msg.gardener_id, msg.action, msg.row, msg.col);

        sem_wait(mutex);
        int done  = sh->garden.done_cells;
        int total = sh->garden.total_work_cells;
        int stop  = sh->garden.stop;
        sem_post(mutex);

        if (stop || done >= total) {
            printf("observer: сад обработан (%d/%d), выхожу\n", done, total);
            break;
        }
    }

    free(buf);
    close_log_queue(mq);
    close_mutex(mutex);
    detach_shared(sh);
    return 0;
}
