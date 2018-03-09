#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char **argv) 
{
	int opt;
    int depth = 0;
    while ((opt = getopt(argc, argv, "d:")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) {
        case 'd':
            depth = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d Depth of binary tree]\n", // In case of wrong options
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    for (; depth > 0; depth--) {
    	if (fork() != 0) { //If is parent, fork again
    		if (fork() != 0) break; //If is parent, break loop after 2 forks
    	}
    }

    printf("look at me I'm a process! \n");


    return 0;
}