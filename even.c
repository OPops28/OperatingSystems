#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Signal handler function
void handle_signal(int signal) {
    if (signal == SIGHUP) {
        printf("Ouch!\n");
    } else if (signal == SIGINT) {
        printf("Yeah!\n");
    }
}

int main(int argc, char *argv[]) {
    // Ensure correct number of command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    // Convert input parameter to integer
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Please provide a positive integer.\n");
        return 1;
    }

    // Register signal handlers
    signal(SIGHUP, handle_signal);
    signal(SIGINT, handle_signal);

    // Print the first n even numbers
    for (int i = 0; i < n; i++) {
        printf("%d\n", i * 2);
        fflush(stdout); // Ensure output is printed before sleeping
        sleep(5);       // Sleep for 5 seconds
    }

    return 0;
}
