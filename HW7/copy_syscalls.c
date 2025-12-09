#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define COPY_BUF_SIZE 32

static void copy_file(int src_fd, int dst_fd) {
    char buffer[COPY_BUF_SIZE];
    ssize_t bytes_read;

    while (1) {
        bytes_read = read(src_fd, buffer, COPY_BUF_SIZE);
        if (bytes_read < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            break;
        }

        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t bytes_written = write(
                dst_fd,
                buffer + total_written,
                (size_t)(bytes_read - total_written)
            );

            if (bytes_written < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            total_written += bytes_written;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <dest_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    if (strcmp(src_path, dst_path) == 0) {
        fprintf(stderr, "Source and destination files must be different.\n");
        return EXIT_FAILURE;
    }

    int src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        perror("open source");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(src_fd, &st) < 0) {
        perror("fstat source");
        close(src_fd);
        return EXIT_FAILURE;
    }

    mode_t src_mode = st.st_mode & 0777;

    int dst_fd = open(dst_path,
                      O_WRONLY | O_CREAT | O_TRUNC,
                      src_mode);
    if (dst_fd < 0) {
        perror("open dest");
        close(src_fd);
        return EXIT_FAILURE;
    }

    copy_file(src_fd, dst_fd);

    if (close(src_fd) < 0) {
        perror("close source");
    }

    if (close(dst_fd) < 0) {
        perror("close dest");
        return EXIT_FAILURE;
    }

    if (chmod(dst_path, src_mode) < 0) {
        perror("chmod dest");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
