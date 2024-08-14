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

    while (1) {
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            continue;
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue;

        background = 0;
        if (line[strlen(line) - 2] == '&') {
            background = 1;
            line[strlen(line) - 2] = '\0';
        }

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: expected argument\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        switch (frkRtnVal = fork()) {
        case -1:
            perror("fork");
            break;

        case 0:
            execvp(v[0], v);
            perror("execvp");
            exit(EXIT_FAILURE);

        default:
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
        }
    }
    return 0;
}
