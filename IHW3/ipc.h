#ifndef IPC_H
#define IPC_H

#include <semaphore.h>
#include <mqueue.h>
#include "garden.h"

#define SHM_NAME "/garden_shm_v2"
#define SEM_NAME "/garden_sem_v2"
#define MQ_NAME  "/garden_log_v1"

typedef struct {
    garden_t garden;
} shared_data_t;

shared_data_t *attach_shared(int create);
void detach_shared(shared_data_t *sh);
void destroy_shared(void);

sem_t *open_mutex(int create);
void close_mutex(sem_t *sem);
void destroy_mutex(void);

typedef struct {
    int  gardener_id;
    int  row;
    int  col;
    char action;
} log_msg_t;

mqd_t open_log_queue(int create);
void close_log_queue(mqd_t mq);
void destroy_log_queue(void);

#endif
