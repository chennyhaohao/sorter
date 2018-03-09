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

    int p[2];
    int c1_rpipe;
    int c2_rpipe;
    int parent_wpipe = 1;
    for (; depth > 0; depth--) {
    	if (pipe(p) == -1) {
    		perror("Pipe error: ");
    		return -1;
    	} 
    	if (fork() != 0) { //If is parent, fork again
    		close(p[1]); //Parent closes writing end
    		c1_rpipe = p[0]; 

    		if (pipe(p) == -1) {
	    		perror("Pipe error: ");
	    		return -1;
	    	} 
    		if (fork() != 0) { //If is parent, break loop after 2 forks
    			close(p[1]);
    			c2_rpipe = p[0];
    			break;
    		} else { //Child 2 closes reading end
    			close(p[0]);
    			parent_wpipe = p[1];
    		}
    	} else {
    		close(p[0]); //Child 1 closes reading end
    		parent_wpipe = p[1];
    	}
    }


    if (depth == 0) {
    	char * msg = "blahblahblahblahblah\n";
    	write(parent_wpipe, msg, strlen(msg));
    	msg = "I'm a sorter!\n";
    	//write(1, msg, strlen(msg));
    	close(parent_wpipe);
    } else {
    	char msg[256];
    	//snprintf(msg, 256, "I'm a merger! %d\n", depth);

    	char c1_buf[256];
    	char c2_buf[256];
    	int c1_read = 0 , c2_read = 0;
    	
    	do {
    		c1_read = read(c1_rpipe, c1_buf, 256);
    		c2_read = read(c2_rpipe, c2_buf, 256);
    		write(parent_wpipe, c1_buf, c1_read);
    		write(parent_wpipe, c2_buf, c2_read);
    	} while(c1_read || c2_read);
    	//snprintf(msg, 256, "I'm a merger! depth: %d c1: %d c2: %d\n", depth, c1_read, c2_read);
	    //write(1, msg, strlen(msg));


    	close(c1_rpipe);
    	close(c2_rpipe);

    	write(parent_wpipe, c1_buf, c1_read);
    	write(parent_wpipe, c2_buf, c2_read);

    	
    	close(parent_wpipe);

    	
    }


    return 0;
}