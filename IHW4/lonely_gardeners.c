#define _XOPEN_SOURCE 700

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef MAX_M
#define MAX_M 50
#endif
#ifndef MAX_N
#define MAX_N 50
#endif

typedef enum {
    CELL_EMPTY   = 0,
    CELL_BLOCKED = 1,
    CELL_DONE_G1 = 2,
    CELL_DONE_G2 = 3
} cell_state_t;

typedef struct {
    cell_state_t state;   // пусто / заблокировано / обработано
    sem_t        occ_sem; // 1 = свободно, 0 = занято
} cell_t;

typedef struct {
    int M, N;

    int blocked_pct;      // фактический процент препятствий
    int blocked_cells;    // кол-во препятствий

    int total_work_cells; // все клетки, которые можно обработать
    int done_cells;       // обработано

    int g1_row, g1_col;   // текущие координаты садовника 1 (или -1)
    int g2_row, g2_col;   // текущие координаты садовника 2 (или -1)

    int stop;             // флаг остановки (по завершению или сигналу)

    cell_t cells[MAX_M][MAX_N];

    pthread_mutex_t stats_mu; // защита done_cells/stop/координат
} garden_t;

typedef struct {
    int id;                 // 1 или 2
    garden_t *g;
    long move_us;           // время «прохода» через любую клетку (единица времени)
    long process_us;        // дополнительное время обработки (больше move_us)
    long wait_poll_us;      // пауза при ожидании занятой клетки
    int  render_every;      // печатать поле каждые N шагов (0 = никогда)
    int  clear_screen;      // очищать экран при печати поля
} gardener_arg_t;

static pthread_mutex_t g_io_mu = PTHREAD_MUTEX_INITIALIZER;
static FILE *g_log_file = NULL;
static int g_log_grid_to_file = 0;

static volatile sig_atomic_t g_sigint_flag = 0;

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
}

static void sleep_us(long usec) {
    if (usec <= 0) return;
    struct timespec ts;
    ts.tv_sec = usec / 1000000L;
    ts.tv_nsec = (usec % 1000000L) * 1000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        // продолжить сон после прерывания
    }
}

static void log_line(const char *fmt, ...) {
    pthread_mutex_lock(&g_io_mu);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);

    if (g_log_file) {
        va_start(ap, fmt);
        vfprintf(g_log_file, fmt, ap);
        fflush(g_log_file);
        va_end(ap);
    }

    pthread_mutex_unlock(&g_io_mu);
}

static int clampi(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static long clampl(long x, long lo, long hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static void print_garden_snapshot(garden_t *g, int clear_screen) {
    pthread_mutex_lock(&g_io_mu);

    if (clear_screen) {
        // ANSI очистка экрана + курсор в (0,0)
        printf("\033[H\033[J");
        fflush(stdout);
    }

    int M, N;
    int done, total;
    int a_r, a_c, b_r, b_c;

    pthread_mutex_lock(&g->stats_mu);
    M = g->M; N = g->N;
    done = g->done_cells;
    total = g->total_work_cells;
    a_r = g->g1_row; a_c = g->g1_col;
    b_r = g->g2_row; b_c = g->g2_col;
    pthread_mutex_unlock(&g->stats_mu);

    printf("Обработано %d из %d (препятствий: %d%% = %d клеток)\n",
           done, total, g->blocked_pct, g->blocked_cells);

    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            char ch;
            int is_a = (r == a_r && c == a_c);
            int is_b = (r == b_r && c == b_c);

            if (is_a && is_b) ch = 'X';
            else if (is_a)   ch = 'A';
            else if (is_b)   ch = 'B';
            else {
                switch (g->cells[r][c].state) {
                    case CELL_BLOCKED: ch = '#'; break;
                    case CELL_EMPTY:   ch = '.'; break;
                    case CELL_DONE_G1: ch = '1'; break;
                    case CELL_DONE_G2: ch = '2'; break;
                    default:           ch = '?'; break;
                }
            }
            putchar(ch);
            putchar(' ');
        }
        putchar('\n');
    }
    putchar('\n');

    fflush(stdout);

    // В файл печатаем «снимок» только если явно разрешено
    if (g_log_file && g_log_grid_to_file) {
        fprintf(g_log_file,
                "Обработано %d из %d (препятствий: %d%% = %d клеток)\n",
                done, total, g->blocked_pct, g->blocked_cells);
        for (int r = 0; r < M; ++r) {
            for (int c = 0; c < N; ++c) {
                char ch;
                int is_a2 = (r == a_r && c == a_c);
                int is_b2 = (r == b_r && c == b_c);

                if (is_a2 && is_b2) ch = 'X';
                else if (is_a2)    ch = 'A';
                else if (is_b2)    ch = 'B';
                else {
                    switch (g->cells[r][c].state) {
                        case CELL_BLOCKED: ch = '#'; break;
                        case CELL_EMPTY:   ch = '.'; break;
                        case CELL_DONE_G1: ch = '1'; break;
                        case CELL_DONE_G2: ch = '2'; break;
                        default:           ch = '?'; break;
                    }
                }
                fputc(ch, g_log_file);
                fputc(' ', g_log_file);
            }
            fputc('\n', g_log_file);
        }
        fputc('\n', g_log_file);
        fflush(g_log_file);
    }

    pthread_mutex_unlock(&g_io_mu);
}

static void on_sigint(int sig) {
    (void)sig;
    g_sigint_flag = 1;
}

// Простой парсер конфигурационного файла: строки вида key=value, комментарии начинаются с '#'.
// Ключи: M, N, move_ms, proc1_ms, proc2_ms, seed, output, render_every, clear, grid_to_file
static int read_config_file(const char *path,
                            int *M, int *N,
                            long *move_us,
                            long *proc1_us,
                            long *proc2_us,
                            unsigned *seed,
                            char *out_path, size_t out_path_cap,
                            int *render_every,
                            int *clear_screen,
                            int *grid_to_file)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Не удалось открыть конфигурационный файл: %s\n", path);
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // убрать комментарий
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';

        // trim
        char *s = line;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        if (*s == '\0') continue;

        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = s;
        char *val = eq + 1;

        // trim key
        char *end = key + strlen(key);
        while (end > key && (end[-1] == ' ' || end[-1] == '\t')) end--;
        *end = '\0';

        // trim val
        while (*val == ' ' || *val == '\t') val++;
        end = val + strlen(val);
        while (end > val && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r')) end--;
        *end = '\0';

        if (strcmp(key, "M") == 0) {
            *M = atoi(val);
        } else if (strcmp(key, "N") == 0) {
            *N = atoi(val);
        } else if (strcmp(key, "move_ms") == 0) {
            *move_us = (long)atol(val) * 1000L;
        } else if (strcmp(key, "proc1_ms") == 0) {
            *proc1_us = (long)atol(val) * 1000L;
        } else if (strcmp(key, "proc2_ms") == 0) {
            *proc2_us = (long)atol(val) * 1000L;
        } else if (strcmp(key, "seed") == 0) {
            *seed = (unsigned)strtoul(val, NULL, 10);
        } else if (strcmp(key, "output") == 0) {
            strncpy(out_path, val, out_path_cap - 1);
            out_path[out_path_cap - 1] = '\0';
        } else if (strcmp(key, "render_every") == 0) {
            *render_every = atoi(val);
        } else if (strcmp(key, "clear") == 0) {
            *clear_screen = atoi(val) ? 1 : 0;
        } else if (strcmp(key, "grid_to_file") == 0) {
            *grid_to_file = atoi(val) ? 1 : 0;
        }
    }

    fclose(f);
    return 1;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Использование:\n"
            "  %s -m M -n N --move-ms X --proc1-ms Y --proc2-ms Z [-o log.txt] [--seed S] [--render-every K] [--no-clear] [--grid-to-file]\n"
            "  %s -c config.txt\n"
            "  %s --interactive\n\n"
            "Пояснения:\n"
            "  -m/-n            размер сада (MxN), максимум %dx%d\n"
            "  препятствия      случайно 10..30%% клеток\n"
            "  --move-ms        время прохода через любую клетку (единица времени)\n"
            "  --proc1-ms       доп. время обработки клетки садовником 1\n"
            "  --proc2-ms       доп. время обработки клетки садовником 2\n"
            "  -o               файл для дублирования вывода\n"
            "  -c               конфигурационный файл key=value\n"
            "  --render-every   печать поля каждые K шагов (0 = не печатать)\n"
            "  --no-clear       не очищать экран при печати поля\n"
            "  --grid-to-file   печатать поле в файл (по умолчанию только события)\n",
            prog, prog, prog, MAX_M, MAX_N);
}

static void garden_destroy(garden_t *g) {
    if (!g) return;
    for (int r = 0; r < g->M; ++r) {
        for (int c = 0; c < g->N; ++c) {
            sem_destroy(&g->cells[r][c].occ_sem);
        }
    }
    pthread_mutex_destroy(&g->stats_mu);
}

static int garden_init(garden_t *g, int M, int N, unsigned seed) {
    if (!g) return 0;

    memset(g, 0, sizeof(*g));

    M = clampi(M, 1, MAX_M);
    N = clampi(N, 1, MAX_N);
    g->M = M;
    g->N = N;

    pthread_mutex_init(&g->stats_mu, NULL);

    // Инициализация клеток
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            g->cells[r][c].state = CELL_EMPTY;
            sem_init(&g->cells[r][c].occ_sem, 0, 1); // свободно
        }
    }

    // Выбираем процент препятствий 10..30 случайно
    srand(seed);
    g->blocked_pct = 10 + (rand() % 21);

    int total_cells = M * N;
    int to_block = (total_cells * g->blocked_pct) / 100;

    // Чтобы стартовые углы точно существовали как клетки сада, не блокируем их.
    // (В условии старт указан фиксированно.)
    int protected_cells = 0;
    bool protect_same = (M == 1 && N == 1);
    (void)protect_same;

    // Ставим препятствия случайно
    g->blocked_cells = 0;
    while (g->blocked_cells < to_block) {
        int r = rand() % M;
        int c = rand() % N;
        if ((r == 0 && c == 0) || (r == M - 1 && c == N - 1)) {
            protected_cells++;
            continue;
        }
        if (g->cells[r][c].state == CELL_EMPTY) {
            g->cells[r][c].state = CELL_BLOCKED;
            g->blocked_cells++;
        }
    }

    g->total_work_cells = 0;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            if (g->cells[r][c].state != CELL_BLOCKED)
                g->total_work_cells++;
        }
    }

    g->done_cells = 0;
    g->stop = 0;

    // Начальные позиции
    g->g1_row = 0;
    g->g1_col = 0;
    g->g2_row = M - 1;
    g->g2_col = N - 1;

    return 1;
}

// Садовник 1: «змейка» по строкам сверху вниз.
static int gardener1_next(const garden_t *g, int *row, int *col, int *dir) {
    int M = g->M, N = g->N;
    if (*dir == +1) {
        if (*col < N - 1) {
            (*col)++;
            return 1;
        }
        if (*row < M - 1) {
            (*row)++;
            *dir = -1;
            return 1;
        }
        return 0;
    } else {
        if (*col > 0) {
            (*col)--;
            return 1;
        }
        if (*row < M - 1) {
            (*row)++;
            *dir = +1;
            return 1;
        }
        return 0;
    }
}

// Садовник 2: «змейка» по столбцам справа налево (движение вверх/вниз).
static int gardener2_next(const garden_t *g, int *row, int *col, int *dir) {
    int M = g->M, N = g->N;
    (void)N;

    if (*dir == -1) { // вверх
        if (*row > 0) {
            (*row)--;
            return 1;
        }
        if (*col > 0) {
            (*col)--;
            *dir = +1;
            return 1;
        }
        return 0;
    } else { // вниз
        if (*row < M - 1) {
            (*row)++;
            return 1;
        }
        if (*col > 0) {
            (*col)--;
            *dir = -1;
            return 1;
        }
        return 0;
    }
}

static int should_stop(garden_t *g) {
    if (g_sigint_flag) {
        pthread_mutex_lock(&g->stats_mu);
        g->stop = 1;
        pthread_mutex_unlock(&g->stats_mu);
        return 1;
    }

    pthread_mutex_lock(&g->stats_mu);
    int stop = g->stop;
    int done = (g->done_cells >= g->total_work_cells);
    if (done) g->stop = 1;
    stop = g->stop;
    pthread_mutex_unlock(&g->stats_mu);

    return stop;
}

static void mark_position(garden_t *g, int id, int row, int col) {
    pthread_mutex_lock(&g->stats_mu);
    if (id == 1) { g->g1_row = row; g->g1_col = col; }
    else         { g->g2_row = row; g->g2_col = col; }
    pthread_mutex_unlock(&g->stats_mu);
}

static int acquire_cell(garden_t *g, int id, int row, int col, long poll_us) {
    int warned = 0;
    for (;;) {
        if (should_stop(g)) return 0;

        if (sem_trywait(&g->cells[row][col].occ_sem) == 0) {
            return 1;
        }

        if (errno != EAGAIN) {
            log_line("[t=%lldms] Садовник %d: ошибка sem_trywait (%d,%d): %s\n",
                     now_ms(), id, row, col, strerror(errno));
            return 0;
        }

        if (!warned) {
            warned = 1;
            log_line("[t=%lldms] Садовник %d ждёт клетку (%d,%d) — там другой садовник\n",
                     now_ms(), id, row, col);
        }

        sleep_us(poll_us);
    }
}

static void release_cell(garden_t *g, int row, int col) {
    sem_post(&g->cells[row][col].occ_sem);
}

static void *gardener_thread(void *argp) {
    gardener_arg_t *arg = (gardener_arg_t *)argp;
    garden_t *g = arg->g;
    int id = arg->id;

    int row, col, dir;
    if (id == 1) {
        row = 0; col = 0; dir = +1;
    } else {
        row = g->M - 1; col = g->N - 1; dir = -1;
    }

    // Старт: занять стартовую клетку (если M=N=1, второй будет ждать)
    if (!acquire_cell(g, id, row, col, arg->wait_poll_us)) {
        mark_position(g, id, -1, -1);
        return NULL;
    }
    mark_position(g, id, row, col);

    log_line("[t=%lldms] Садовник %d стартовал в (%d,%d)\n", now_ms(), id, row, col);

    int step = 0;
    for (;;) {
        if (should_stop(g)) {
            release_cell(g, row, col);
            mark_position(g, id, -1, -1);
            log_line("[t=%lldms] Садовник %d завершает работу (остановка)\n", now_ms(), id);
            return NULL;
        }

        // Мы находимся в (row,col), клетка занята нами.
        // Если клетка доступна и еще не обработана — обработать.
        int processed_now = 0;
        if (g->cells[row][col].state == CELL_EMPTY) {
            g->cells[row][col].state = (id == 1) ? CELL_DONE_G1 : CELL_DONE_G2;

            pthread_mutex_lock(&g->stats_mu);
            g->done_cells++;
            int done = g->done_cells;
            int total = g->total_work_cells;
            if (done >= total) g->stop = 1;
            pthread_mutex_unlock(&g->stats_mu);

            processed_now = 1;
            log_line("[t=%lldms] Садовник %d обработал клетку (%d,%d)\n", now_ms(), id, row, col);
        } else if (g->cells[row][col].state == CELL_BLOCKED) {
            log_line("[t=%lldms] Садовник %d проходит препятствие (%d,%d)\n", now_ms(), id, row, col);
        } else {
            // уже обработано другим
            log_line("[t=%lldms] Садовник %d проходит уже обработанную клетку (%d,%d)\n", now_ms(), id, row, col);
        }

        // Проход через клетку занимает единицу времени
        sleep_us(arg->move_us);
        // Обработка занимает дополнительное время
        if (processed_now) sleep_us(arg->process_us);

        step++;
        if (arg->render_every > 0 && (step % arg->render_every) == 0) {
            print_garden_snapshot(g, arg->clear_screen);
        }

        if (should_stop(g)) {
            release_cell(g, row, col);
            mark_position(g, id, -1, -1);
            log_line("[t=%lldms] Садовник %d завершает работу (все клетки обработаны или сигнал)\n",
                     now_ms(), id);
            return NULL;
        }

        // Переход к следующей клетке по маршруту
        int nr = row, nc = col, nd = dir;
        int ok = (id == 1)
                    ? gardener1_next(g, &nr, &nc, &nd)
                    : gardener2_next(g, &nr, &nc, &nd);

        if (!ok) {
            // Маршрут закончился
            release_cell(g, row, col);
            mark_position(g, id, -1, -1);
            log_line("[t=%lldms] Садовник %d дошел до конца маршрута и завершил работу\n", now_ms(), id);
            return NULL;
        }

        // Освобождаем текущую клетку и пытаемся занять следующую.
        release_cell(g, row, col);

        if (!acquire_cell(g, id, nr, nc, arg->wait_poll_us)) {
            mark_position(g, id, -1, -1);
            return NULL;
        }

        row = nr; col = nc; dir = nd;
        mark_position(g, id, row, col);
        log_line("[t=%lldms] Садовник %d перешёл в (%d,%d)\n", now_ms(), id, row, col);
    }
}

// -------------------- main / разбор параметров --------------------

typedef struct {
    int M, N;
    long move_us;
    long proc1_us;
    long proc2_us;
    unsigned seed;

    char out_path[256];
    int have_out;

    int render_every;
    int clear_screen;
    int grid_to_file;

    int interactive;
    char cfg_path[256];
    int have_cfg;
} options_t;

static void set_defaults(options_t *o) {
    memset(o, 0, sizeof(*o));
    o->M = 10;
    o->N = 10;
    o->move_us  = 200L * 1000L; // 200ms
    o->proc1_us = 800L * 1000L; // 800ms
    o->proc2_us = 900L * 1000L; // 900ms
    o->seed = (unsigned)time(NULL);
    o->render_every = 5;
    o->clear_screen = 0;
    o->grid_to_file = 0;
}

static int parse_args(int argc, char **argv, options_t *o) {
    set_defaults(o);

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];

        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            return 0;
        } else if (strcmp(a, "--interactive") == 0) {
            o->interactive = 1;
        } else if (strcmp(a, "-c") == 0 && i + 1 < argc) {
            strncpy(o->cfg_path, argv[++i], sizeof(o->cfg_path) - 1);
            o->cfg_path[sizeof(o->cfg_path) - 1] = '\0';
            o->have_cfg = 1;
        } else if ((strcmp(a, "-m") == 0 || strcmp(a, "--m") == 0) && i + 1 < argc) {
            o->M = atoi(argv[++i]);
        } else if ((strcmp(a, "-n") == 0 || strcmp(a, "--n") == 0) && i + 1 < argc) {
            o->N = atoi(argv[++i]);
        } else if (strcmp(a, "--move-ms") == 0 && i + 1 < argc) {
            o->move_us = (long)atol(argv[++i]) * 1000L;
        } else if (strcmp(a, "--proc1-ms") == 0 && i + 1 < argc) {
            o->proc1_us = (long)atol(argv[++i]) * 1000L;
        } else if (strcmp(a, "--proc2-ms") == 0 && i + 1 < argc) {
            o->proc2_us = (long)atol(argv[++i]) * 1000L;
        } else if (strcmp(a, "--seed") == 0 && i + 1 < argc) {
            o->seed = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(a, "-o") == 0 && i + 1 < argc) {
            strncpy(o->out_path, argv[++i], sizeof(o->out_path) - 1);
            o->out_path[sizeof(o->out_path) - 1] = '\0';
            o->have_out = 1;
        } else if (strcmp(a, "--render-every") == 0 && i + 1 < argc) {
            o->render_every = atoi(argv[++i]);
        } else if (strcmp(a, "--no-clear") == 0) {
            o->clear_screen = 0;
        } else if (strcmp(a, "--clear") == 0) {
            o->clear_screen = 1;
        } else if (strcmp(a, "--grid-to-file") == 0) {
            o->grid_to_file = 1;
        } else {
            fprintf(stderr, "Неизвестный аргумент: %s\n", a);
            return 0;
        }
    }

    return 1;
}

static void interactive_read(options_t *o) {
    printf("Введите M (1..%d): ", MAX_M);
    fflush(stdout);
    scanf("%d", &o->M);

    printf("Введите N (1..%d): ", MAX_N);
    fflush(stdout);
    scanf("%d", &o->N);

    long move_ms, p1_ms, p2_ms;
    printf("Введите время прохода (move_ms): ");
    fflush(stdout);
    scanf("%ld", &move_ms);

    printf("Введите доп. время обработки для садовника 1 (proc1_ms): ");
    fflush(stdout);
    scanf("%ld", &p1_ms);

    printf("Введите доп. время обработки для садовника 2 (proc2_ms): ");
    fflush(stdout);
    scanf("%ld", &p2_ms);

    o->move_us = move_ms * 1000L;
    o->proc1_us = p1_ms * 1000L;
    o->proc2_us = p2_ms * 1000L;

    printf("Введите seed (0 = текущий time): ");
    fflush(stdout);
    unsigned s;
    scanf("%u", &s);
    if (s != 0) o->seed = s;

    printf("Файл лога (пусто = не писать): ");
    fflush(stdout);

    // аккуратно дочитать перевод строки после scanf
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}

    char buf[256];
    if (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        if (len > 0) {            snprintf(o->out_path, sizeof(o->out_path), "%s", buf);
            o->have_out = 1;
        }
    }
}

int main(int argc, char **argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    options_t opt;
    if (!parse_args(argc, argv, &opt)) {
        usage(argv[0]);
        return 1;
    }

    // Если ничего не задано — интерактивный режим (чтобы соответствовать варианту ввода с консоли).
    if (argc == 1) opt.interactive = 1;

    if (opt.have_cfg) {
        // конфиг заменяет параметры командной строки
        int M = opt.M, N = opt.N;
        long move_us = opt.move_us, p1 = opt.proc1_us, p2 = opt.proc2_us;
        unsigned seed = opt.seed;
        char out_path[256] = {0};
        int render_every = opt.render_every;
        int clear_screen = opt.clear_screen;
        int grid_to_file = opt.grid_to_file;

        if (!read_config_file(opt.cfg_path,
                              &M, &N,
                              &move_us, &p1, &p2,
                              &seed,
                              out_path, sizeof(out_path),
                              &render_every,
                              &clear_screen,
                              &grid_to_file)) {
            return 1;
        }

        opt.M = M; opt.N = N;
        opt.move_us = move_us;
        opt.proc1_us = p1;
        opt.proc2_us = p2;
        opt.seed = seed;
        opt.render_every = render_every;
        opt.clear_screen = clear_screen;
        opt.grid_to_file = grid_to_file;

        if (out_path[0] != '\0') {            snprintf(opt.out_path, sizeof(opt.out_path), "%s", out_path);
            opt.have_out = 1;
        }
    }

    if (opt.interactive) {
        interactive_read(&opt);
    }

    // Нормализация параметров
    opt.M = clampi(opt.M, 1, MAX_M);
    opt.N = clampi(opt.N, 1, MAX_N);

    // Важно: move < process по условию (move — единица времени, меньше времени обработки)
    if (opt.move_us <= 0) opt.move_us = 1000L;

    if (opt.proc1_us < opt.move_us) {
        opt.proc1_us = opt.move_us * 2;
    }
    if (opt.proc2_us < opt.move_us) {
        opt.proc2_us = opt.move_us * 2;
    }

    // Открыть лог-файл
    if (opt.have_out) {
        g_log_file = fopen(opt.out_path, "w");
        if (!g_log_file) {
            fprintf(stderr, "Не удалось открыть файл лога: %s\n", opt.out_path);
            return 1;
        }
    }
    g_log_grid_to_file = opt.grid_to_file;

    garden_t g;
    if (!garden_init(&g, opt.M, opt.N, opt.seed)) {
        fprintf(stderr, "Ошибка инициализации сада\n");
        if (g_log_file) fclose(g_log_file);
        return 1;
    }

    log_line("Параметры: M=%d, N=%d, move=%ldms, proc1=%ldms, proc2=%ldms, seed=%u\n",
             opt.M, opt.N,
             opt.move_us / 1000L,
             opt.proc1_us / 1000L,
             opt.proc2_us / 1000L,
             opt.seed);

    print_garden_snapshot(&g, opt.clear_screen);

    gardener_arg_t a1 = {
        .id = 1,
        .g = &g,
        .move_us = opt.move_us,
        .process_us = opt.proc1_us,
        .wait_poll_us = clampl(opt.move_us / 3, 1000L, 200000L),
        .render_every = opt.render_every,
        .clear_screen = opt.clear_screen
    };

    gardener_arg_t a2 = {
        .id = 2,
        .g = &g,
        .move_us = opt.move_us,
        .process_us = opt.proc2_us,
        .wait_poll_us = clampl(opt.move_us / 3, 1000L, 200000L),
        .render_every = opt.render_every,
        .clear_screen = opt.clear_screen
    };

    pthread_t t1, t2;
    if (pthread_create(&t1, NULL, gardener_thread, &a1) != 0) {
        fprintf(stderr, "Не удалось создать поток садовника 1\n");
        garden_destroy(&g);
        if (g_log_file) fclose(g_log_file);
        return 1;
    }
    if (pthread_create(&t2, NULL, gardener_thread, &a2) != 0) {
        fprintf(stderr, "Не удалось создать поток садовника 2\n");
        pthread_mutex_lock(&g.stats_mu);
        g.stop = 1;
        pthread_mutex_unlock(&g.stats_mu);
        pthread_join(t1, NULL);
        garden_destroy(&g);
        if (g_log_file) fclose(g_log_file);
        return 1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    print_garden_snapshot(&g, 0);

    pthread_mutex_lock(&g.stats_mu);
    int done = g.done_cells;
    int total = g.total_work_cells;
    pthread_mutex_unlock(&g.stats_mu);

    if (g_sigint_flag) {
        log_line("Итог: остановлено по сигналу. Обработано %d из %d.\n", done, total);
    } else {
        log_line("Итог: работа завершена. Обработано %d из %d.\n", done, total);
    }

    garden_destroy(&g);
    if (g_log_file) fclose(g_log_file);
    return 0;
}
