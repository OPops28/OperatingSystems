#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20 /* max number of command tokens */
#define NL 100 /* input buffer size */

char line[NL]; /* command input buffer */
int bg_jobs = 0; /* background job counter */

/* shell prompt */
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

/* signal handler for background processes */
void bg_handler(int sig) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("\n[%d]+ Done %d\n", bg_jobs, pid);
        bg_jobs--;
        prompt();  // Reprompt after background process finishes
    }
}

/* main function */
int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal; /* value returned by fork sys call */
    int wpid; /* value returned by wait */
    char *v[NV]; /* array of pointers to command line tokens */
    char *sep = " \t\n"; /* command line token separators */
    int i; /* parse index */
    int bg_flag; /* background process flag */

    /* Install signal handler for background processes */
    signal(SIGCHLD, bg_handler);

    /* prompt for and process one command line at a time */
    while (1) { /* do Forever */
        prompt();
        fgets(line, NL, stdin);

        if (feof(stdin)) { /* non-zero on EOF */
            fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
            exit(0);
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue; /* to prompt */

        /* tokenize the input */
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        /* check if command should be run in background */
        bg_flag = (i > 1 && strcmp(v[i-1], "&") == 0);
        if (bg_flag) {
            v[i-1] = NULL;  // remove the '&' from the arguments
            bg_jobs++; // increment the background job counter
        }

        /* check for built-in commands */
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: missing operand\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("cd failed");
                }
            }
            continue; /* go back to prompt */
        }

        /* fork a child process to exec the command in v[0] */
        frkRtnVal = fork();
        if (frkRtnVal < 0) {
            perror("fork failed");
            continue;
        }

        if (frkRtnVal == 0) { /* code executed only by child process */
            execvp(v[0], v);
            /* if execvp returns, it must have failed */
            perror("execvp failed");
            exit(1);
        }

        if (!bg_flag) { /* code executed only by parent process */
            wpid = waitpid(frkRtnVal, NULL, 0);
            if (wpid < 0) {
                perror("waitpid failed");
            } else {
                printf("%s done \n", v[0]);
            }
        }
    } /* while */
    return 0;
} /* main */

