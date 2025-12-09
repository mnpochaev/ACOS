#ifndef RUNNER_H
#define RUNNER_H

#include <semaphore.h>
#include <unistd.h>
#include "ipc.h"

void print_garden_safe(shared_data_t *sh, sem_t *mutex);
void run_gardener(int id,
                  shared_data_t *sh,
                  sem_t *mutex,
                  useconds_t move_time_us,
                  useconds_t process_time_us);

#endif
