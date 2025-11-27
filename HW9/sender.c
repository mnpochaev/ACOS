#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t ack_received = 0;
pid_t receiver_pid = 0;

void ack_handler(int sig) {
    (void)sig;
    ack_received = 1;
}

int main(void) {
    pid_t my_pid = getpid();
    printf("Sender PID = %d\n", my_pid);

    printf("Input receiver PID: ");
    fflush(stdout);
    if (scanf("%d", &receiver_pid) != 1) {
        fprintf(stderr, "Error: cannot read receiver PID\n");
        return 1;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = ack_handler;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        return 1;
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        return 1;
    }

    int32_t value;
    printf("Input decimal integer number: ");
    fflush(stdout);
    if (scanf("%d", &value) != 1) {
        fprintf(stderr, "Error: cannot read integer\n");
        return 1;
    }

    uint32_t u = (uint32_t)value;

    for (int i = 31; i >= 0; --i) {
        int bit = (u >> i) & 1u;
        int sig = bit ? SIGUSR2 : SIGUSR1;

        ack_received = 0;

        if (kill(receiver_pid, sig) == -1) {
            perror("kill (send bit)");
            return 1;
        }

        printf("%c", bit ? '1' : '0');
        fflush(stdout);

        while (!ack_received) {
            pause();
        }

        usleep(100000);
    }
    printf("\n");

    if (kill(receiver_pid, SIGINT) == -1) {
        perror("kill (SIGINT)");
        return 1;
    }

    printf("Result = %d\n", value);

    return 0;
}

