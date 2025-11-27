#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t bits_received = 0;
volatile uint32_t received_u = 0;
volatile sig_atomic_t finished = 0;

pid_t sender_pid = 0;

void send_ack(void) {
    if (sender_pid > 0) {
        kill(sender_pid, SIGUSR1);
    }
}

void bit0_handler(int sig) {
    (void)sig;
    received_u = (received_u << 1);
    bits_received++;

    write(STDOUT_FILENO, "0", 1);

    send_ack();
}

void bit1_handler(int sig) {
    (void)sig;
    received_u = (received_u << 1) | 1u;
    bits_received++;

    write(STDOUT_FILENO, "1", 1);

    send_ack();
}

void finish_handler(int sig) {
    (void)sig;
    finished = 1;
}

int main(void) {
    pid_t my_pid = getpid();
    printf("Receiver PID = %d\n", my_pid);

    printf("Input sender PID: ");
    fflush(stdout);
    if (scanf("%d", &sender_pid) != 1) {
        fprintf(stderr, "Error: cannot read sender PID\n");
        return 1;
    }

    struct sigaction sa0, sa1, safin;

    sigemptyset(&sa0.sa_mask);
    sa0.sa_flags = 0;
    sa0.sa_handler = bit0_handler;
    if (sigaction(SIGUSR1, &sa0, NULL) == -1) {
        perror("sigaction SIGUSR1");
        return 1;
    }

    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;
    sa1.sa_handler = bit1_handler;
    if (sigaction(SIGUSR2, &sa1, NULL) == -1) {
        perror("sigaction SIGUSR2");
        return 1;
    }

    sigemptyset(&safin.sa_mask);
    safin.sa_flags = 0;
    safin.sa_handler = finish_handler;
    if (sigaction(SIGINT, &safin, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }

    while (!finished) {
        pause();
    }

    write(STDOUT_FILENO, "\n", 1);

    int32_t value = (int32_t)received_u;
    printf("Result = %d\n", value);

    return 0;
}
