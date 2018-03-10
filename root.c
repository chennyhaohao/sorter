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
    char buf[1024];
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
    int p[2];
    if (pipe(p) == -1) {
        perror("Root pipe error: ");
        return -1;
    }

    if (fork() != 0) {
        //printf("I'm the root!\n");
        close(p[1]); //Parent closes writing end
        int b_read;
        do {
            b_read = read(p[0], buf, 1024);
            if (b_read > 0) {
                write(1, buf, b_read);
            }
        } while(b_read > 0);
        close(p[0]);
        printf("Look at me! I'm the root!\n");

    } else {
        close(p[0]); //Child closes reading end
        if(dup2(p[1], 1) < 0) { //Redirect sdout to pipe
            perror("Dup2 error: ");
            return -1;
        } 
        if (execlp("./node", "./node", "-d", depth, NULL) == -1) perror("Exec failed: ");
    }

    return 0;
}