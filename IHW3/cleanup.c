#define _XOPEN_SOURCE 700

#include <stdio.h>
#include "ipc.h"

int main(void)
{
    printf("cleanup: удаляю разделяемую память, семафор и очередь сообщений...\n");

    destroy_shared();
    destroy_mutex();
    destroy_log_queue();

    printf("cleanup: если ошибок не было выше, ресурсы удалены.\n");
    return 0;
}

