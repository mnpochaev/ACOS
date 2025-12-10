#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define N_PRODUCERS 100
#define BUF_CAP     1000

typedef struct {
    int data[BUF_CAP];
    int size;
} buffer_t;

buffer_t buffer = { .size = 0 };

pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  buf_cond  = PTHREAD_COND_INITIALIZER;

int producers_left = N_PRODUCERS;
int active_summers = 0;
int final_result   = 0;
int next_sum_id    = 0;

static int rand_range(unsigned int *seed, int from, int to) {
    return from + (rand_r(seed) % (to - from + 1));
}

static void buffer_push(int x) {
    if (buffer.size >= BUF_CAP) {
        fprintf(stderr, "Ошибка: переполнение буфера!\n");
        exit(1);
    }
    buffer.data[buffer.size++] = x;
}

static int buffer_pop(void) {
    if (buffer.size <= 0) {
        fprintf(stderr, "Ошибка: чтение из пустого буфера!\n");
        exit(1);
    }
    return buffer.data[--buffer.size];
}

static void print_buffer(void) {
    printf("    [буфер, элементов=%d] ", buffer.size);
    for (int i = 0; i < buffer.size; ++i)
        printf("%d ", buffer.data[i]);
    printf("\n");
}

typedef struct {
    int a, b;
    int id;
} sum_args_t;

void *summer_thread(void *arg) {
    sum_args_t *sa = (sum_args_t *)arg;
    unsigned int seed = time(NULL) ^ (sa->id * 1234567u);

    int a = sa->a, b = sa->b, id = sa->id;
    free(sa);

    int sleep_time = rand_range(&seed, 3, 6);
    printf("[СУМ %3d] старт: %d + %d, время работы %d c\n",
           id, a, b, sleep_time);
    sleep(sleep_time);
    int s = a + b;

    pthread_mutex_lock(&buf_mutex);
    active_summers--;
    buffer_push(s);
    printf("[СУМ %3d] готово: %d + %d = %d -> в буфер\n",
           id, a, b, s);
    print_buffer();
    pthread_cond_broadcast(&buf_cond);
    pthread_mutex_unlock(&buf_mutex);

    return NULL;
}

void *producer_thread(void *arg) {
    long id_l = (long)arg;
    int id = (int)id_l;
    unsigned int seed = time(NULL) ^ (id * 1103515245u);

    int sleep_time = rand_range(&seed, 1, 7);
    int value      = rand_range(&seed, 1, 100);

    sleep(sleep_time);

    pthread_mutex_lock(&buf_mutex);
    buffer_push(value);
    producers_left--;
    printf("[ИСТ %3d] сгенерировал %3d за %d c -> в буфер "
           "(источников осталось=%d)\n",
           id, value, sleep_time, producers_left);
    print_buffer();
    pthread_cond_broadcast(&buf_cond);
    pthread_mutex_unlock(&buf_mutex);

    return NULL;
}

void *watcher_thread(void *arg) {
    (void)arg;
    int step = 0;

    while (1) {
        pthread_mutex_lock(&buf_mutex);
        while (buffer.size < 2 &&
               !(buffer.size == 1 &&
                 producers_left == 0 &&
                 active_summers == 0)) {
            pthread_cond_wait(&buf_cond, &buf_mutex);
        }

        if (buffer.size == 1 && producers_left == 0 && active_summers == 0) {
            final_result = buffer.data[0];
            printf("[НАБЛЮД] окончательный результат = %d\n", final_result);
            pthread_mutex_unlock(&buf_mutex);
            break;
        }

        int a = buffer_pop();
        int b = buffer_pop();

        int sum_id = ++next_sum_id;
        active_summers++;
        step++;

        printf("[НАБЛЮД] шаг %d: беру %d и %d для СУМ %d\n",
               step, a, b, sum_id);
        print_buffer();

        sum_args_t *sa = malloc(sizeof(*sa));
        sa->a = a;
        sa->b = b;
        sa->id = sum_id;

        pthread_t tid;
        pthread_create(&tid, NULL, summer_thread, sa);
        pthread_detach(tid);

        pthread_mutex_unlock(&buf_mutex);
    }

    return NULL;
}

int main(void) {
    pthread_t producers[N_PRODUCERS];
    pthread_t watcher;

    printf("Старт. Количество источников: %d\n", N_PRODUCERS);

    pthread_create(&watcher, NULL, watcher_thread, NULL);

    for (long i = 0; i < N_PRODUCERS; ++i)
        pthread_create(&producers[i], NULL, producer_thread, (void *)i);

    for (int i = 0; i < N_PRODUCERS; ++i)
        pthread_join(producers[i], NULL);

    pthread_join(watcher, NULL);

    printf("main: вычисления завершены, результат = %d\n", final_result);
    return 0;
}
