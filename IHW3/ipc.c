#define _XOPEN_SOURCE 700
#include "ipc.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mqueue.h>


shared_data_t *attach_shared(int create)
{
    int fd;

    if (create) {
        fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd == -1 && errno == EEXIST) {
            shm_unlink(SHM_NAME);
            fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
        }
    } else {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
    }

    if (fd == -1) {
        perror("shm_open");
        return NULL;
    }

    if (create) {
        if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
            perror("ftruncate");
            close(fd);
            shm_unlink(SHM_NAME);
            return NULL;
        }
    }

    void *ptr = mmap(NULL, sizeof(shared_data_t),
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    close(fd);

    if (ptr == MAP_FAILED) {
        perror("mmap");
        if (create) shm_unlink(SHM_NAME);
        return NULL;
    }

    return (shared_data_t *)ptr;
}

void detach_shared(shared_data_t *sh)
{
    if (sh) {
        munmap(sh, sizeof(shared_data_t));
    }
}

void destroy_shared(void)
{
    shm_unlink(SHM_NAME);
}


sem_t *open_mutex(int create)
{
    sem_t *sem;

    if (create) {
        sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
        if (sem == SEM_FAILED && errno == EEXIST) {
            sem_unlink(SEM_NAME);
            sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
        }
    } else {
        sem = sem_open(SEM_NAME, 0);
    }

    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }
    return sem;
}

void close_mutex(sem_t *sem)
{
    if (sem) sem_close(sem);
}

void destroy_mutex(void)
{
    sem_unlink(SEM_NAME);
}

mqd_t open_log_queue(int create)
{
    mqd_t mq;

    if (create) {
        mq_unlink(MQ_NAME);

        mq = mq_open(MQ_NAME,
                     O_CREAT | O_RDONLY,
                     0666,
                     NULL);
    } else {
        mq = mq_open(MQ_NAME, O_WRONLY | O_NONBLOCK);
    }

    if (mq == (mqd_t)-1) {
        perror("mq_open");
    }

    return mq;
}


void close_log_queue(mqd_t mq)
{
    if (mq != (mqd_t)-1) {
        mq_close(mq);
    }
}

void destroy_log_queue(void)
{
    mq_unlink(MQ_NAME);
}
