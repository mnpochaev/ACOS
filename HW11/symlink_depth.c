#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int keep = 0;
    if (argc >= 2 && strcmp(argv[1], "--keep") == 0) {
        keep = 1;
    }

    char dir_template[] = "/tmp/symlink_depth.XXXXXX";
    char *workdir = mkdtemp(dir_template);
    if (!workdir) die("mkdtemp");

    printf("Рабочая директория: %s\n", workdir);

    if (chdir(workdir) != 0) die("chdir");

    // 1) создаём регулярный файл "a"
    const char *base = "a";
    int fd = open(base, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0644);
    if (fd < 0) die("open(base)");
    if (close(fd) != 0) die("close(base)");

    // Динамический список имён созданных симлинков (для уборки)
    size_t cap = 64, nlinks = 0;
    char **links = (char **)calloc(cap, sizeof(char *));
    if (!links) die("calloc");

    const char *prev = base;
    int max_ok = 0;
    int attempt = 1;

    for (;;) {
        char name[64];
        snprintf(name, sizeof(name), "link_%03d", attempt);

        if (symlink(prev, name) != 0) {
            fprintf(stderr, "Не удалось создать симлинк %s -> %s: %s (%d)\n",
                    name, prev, strerror(errno), errno);
            break;
        }

        if (nlinks == cap) {
            cap *= 2;
            char **tmp = (char **)realloc(links, cap * sizeof(char *));
            if (!tmp) die("realloc");
            links = tmp;
        }
        links[nlinks] = strdup(name);
        if (!links[nlinks]) die("strdup");
        nlinks++;

        fd = open(name, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            int e = errno;
            printf("ПРОВАЛ на глубине %d: open(\"%s\") -> %s (%d)\n",
                   attempt, name, strerror(e), e);
            printf("Максимальная глубина, при которой open() ещё успешен: %d\n", max_ok);
            if (e == ELOOP) {
                printf("Причина ожидаемая: превышен лимит переходов по симлинкам (ELOOP).\n");
            } else {
                printf("Причина не ELOOP — проверьте права/ФС/ограничения: %s (%d)\n", strerror(e), e);
            }
            break;
        }

        if (close(fd) != 0) die("close(opened)");

        max_ok = attempt;
        prev = links[nlinks - 1]; // предыдущая цель — только что созданный симлинк
        attempt++;
    }

    // 2) уборка
    if (!keep) {
        for (ssize_t i = (ssize_t)nlinks - 1; i >= 0; --i) {
            if (links[i]) unlink(links[i]);
        }
        unlink(base);

        // выходим из директории перед удалением
        if (chdir("/") != 0) die("chdir(/)");

        if (rmdir(workdir) != 0) {
            fprintf(stderr, "Предупреждение: не удалось удалить директорию %s: %s\n",
                    workdir, strerror(errno));
        }
    } else {
        printf("Файлы оставлены (режим --keep). Удалить можно так:\n");
        printf("  rm -rf %s\n", workdir);
    }

    for (size_t i = 0; i < nlinks; ++i) free(links[i]);
    free(links);

    return 0;
}