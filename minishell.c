#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20   /* max number of command tokens */
#define NL 100  /* input buffer size */

char line[NL]; /* command input buffer */

typedef struct {
    int pid;
    int job_number;
    char command[NL];
} BackgroundJob;

BackgroundJob background_jobs[NV];
int bg_job_count = 0; /* Number of background jobs */

/* Function to display shell prompt */
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

/* Function to handle background jobs */
void handle_background_jobs() {
    int status;
    for (int i = 0; i < bg_job_count; i++) {
        int wpid = waitpid(background_jobs[i].pid, &status, WNOHANG);
        if (wpid > 0) {
            /* Background job finished */
            printf("[%d]+ Done %s\n", background_jobs[i].job_number, background_jobs[i].command);
            /* Remove job from list */
            for (int j = i; j < bg_job_count - 1; j++) {
                background_jobs[j] = background_jobs[j + 1];
            }
            bg_job_count--;
            i--;
        }
    }
}

int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal;  /* value returned by fork sys call */
    int wpid;       /* value returned by wait */
    char *v[NV];    /* array of pointers to command line tokens */
    char *sep = " \t\n"; /* command line token separators */
    int i;          /* parse index */
    int background; /* flag for background execution */
    int job_number; /* Background job number */

    while (1) {
        prompt();

        /* Read the command line input */
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) { /* non-zero on EOF */
                fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            continue;
        }

        /* Ignore comments, empty lines, and null commands */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue;

        /* Check if the command should be run in the background */
        background = 0;
        size_t len = strlen(line);
        if (len > 1 && line[len - 2] == '&') {
            background = 1;
            line[len - 2] = '\0';  /* Remove '&' from the command */
        }

        /* Tokenize the input command line */
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        /* Handle built-in commands */
        if (v[0] != NULL && strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("cd");  /* Error handling for chdir */
                }
            }
            continue;  /* Return to prompt */
        }

        /* Handle background jobs before executing new command */
        handle_background_jobs();

        /* Fork a new process to execute the command */
        switch (frkRtnVal = fork()) {
            case -1:  /* Fork error */
                perror("fork");
                break;

            case 0:   /* Child process */
                execvp(v[0], v);
                perror("execvp");  /* Execvp failed, report error */
                exit(EXIT_FAILURE);  /* Exit child process if exec fails */

            default:  /* Parent process */
                if (background) {
                    /* Background process - print message and add to background jobs list */
                    job_number = bg_job_count + 1;
                    background_jobs[bg_job_count].pid = frkRtnVal;
                    background_jobs[bg_job_count].job_number = job_number;
                    strncpy(background_jobs[bg_job_count].command, v[0], NL);
                    bg_job_count++;
                    printf("[%d] %d\n", job_number, frkRtnVal);
                } else {
                    /* Foreground process - wait for it to complete */
                    wpid = waitpid(frkRtnVal, NULL, 0);
                    if (wpid == -1) {
                        perror("waitpid");
                    } else {
                        printf("%s done\n", v[0]);
                    }
                }
                break;
        }
    }
    return 0;
}
