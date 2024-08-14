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

/* shell prompt */
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

int main(int argc, char *argv[], char *envp[]) {
    int frkRtnVal;  /* value returned by fork sys call */
    int wpid;       /* value returned by wait */
    char *v[NV];    /* array of pointers to command line tokens */
    char *sep = " \t\n"; /* command line token separators */
    int i;          /* parse index */
    int background; /* flag for background execution */

    /* prompt for and process one command line at a time */
    while (1) { /* do Forever */
        prompt();
        if (fgets(line, NL, stdin) == NULL) { /* handle EOF */
            if (feof(stdin)) {
                fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            continue;
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue; /* ignore empty lines and comments */

        background = 0; /* reset background flag */
        if (line[strlen(line) - 2] == '&') { /* check if command ends with '&' */
            background = 1;
            line[strlen(line) - 2] = '\0'; /* remove '&' from command */
        }

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        /* Check for built-in commands */
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: expected argument\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("cd");
                }
            }
            continue; /* return to prompt */
        }

        /* Fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
        case -1: /* fork returns error to parent process */
            perror("fork");
            break;

        case 0: /* code executed only by child process */
            execvp(v[0], v);
            perror("execvp"); /* execvp failed */
            exit(EXIT_FAILURE); /* terminate child process */

        default: /* code executed only by parent process */
            if (background) {
                printf("[%d] %s running in background\n", frkRtnVal, v[0]);
            } else {
                wpid = waitpid(frkRtnVal, NULL, 0);
                if (wpid == -1) {
                    perror("waitpid");
                } else {
                    printf("%s done\n", v[0]);
                }
            }
            break;
        } /* switch */
    } /* while */
    return 0;
} /* main */
