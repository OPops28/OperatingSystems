#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void handle_sighup(int sig) {
    printf("Ouch!\n");
}

void handle_sigint(int sig) {
    printf("Yeah!\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Please provide a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    // Set up signal handlers
    struct sigaction sa_sighup, sa_sigint;
    sa_sighup.sa_handler = handle_sighup;
    sigemptyset(&sa_sighup.sa_mask);
    sa_sighup.sa_flags = 0;
    sigaction(SIGHUP, &sa_sighup, NULL);

    sa_sigint.sa_handler = handle_sigint;
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigint.sa_flags = 0;
    sigaction(SIGINT, &sa_sigint, NULL);

    for (int i = 0; i < n; i++) {
        printf("%d\n", i * 2);
        sleep(5);
    }

    return 0;
}
