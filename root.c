#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main(int argc, char **argv) 
{
	int opt;
    char depth[128];
    while ((opt = getopt(argc, argv, "d:")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) {
        case 'd':
            strcpy(depth, optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d Depth of binary tree]\n", // In case of wrong options
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (fork() != 0) {
        printf("I'm the root!\n");
    } else {
        if (execlp("./node", "./node", "-d", depth, NULL) == -1) perror("Exec failed: ");
    }

    return 0;
}