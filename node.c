#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "./record.c"
#include "./merge.h"

#define BUF_SIZE 4096

int r_rand(int rmin, int rmax) { //Returns random number between rmin and rmax (inclusive)
    if (rmax <= rmin) return rmin;
    return rand()%(rmax - rmin + 1) + rmin;
}

int main(int argc, char **argv) 
{	
	int attr_num, depth, rand_range = 0;
	int argNum = 0;
	int r_start = 0, r_end, r_mid;
	int opt;
    int id_start = 0, id_range;
	char * usage_msg = "Usage: %s [-d Depth of binary tree] [-a Attribute Number] [-f file to sort] [-n number of records] [-r]\n";
    char parent_pipe_name[64], ifile[256];
    while ((opt = getopt(argc, argv, "d:a:o:f:n:r")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) { 
        case 'd':
            depth = atoi(optarg);
            if (depth < 0 || depth > 6) {
                printf("Invalid Depth.\n");
                return -1;
            }
            argNum++;
            id_range = (int)(pow(2, depth) + 0.5);
            break;

        case 'a':
        	attr_num = atoi(optarg);
            if (attr_num < 0 || attr_num > 3) {
                printf("Invalid Attribute Number.\n");
                return -1;
            }
        	argNum++;
        	break;
        case 'o':
            snprintf(parent_pipe_name, 64, "%s", optarg);
            break;

        case 'f':
            snprintf(ifile, 256, "%s", optarg);
            argNum++;
            break;

        case 'n':
            r_end = atoi(optarg) - 1;
            argNum++;
            break;

        case 'r':
            rand_range = 1;
            break;

        default: 
            fprintf(stderr, usage_msg, // In case of wrong options
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (argNum < 4) {
    	fprintf(stderr, usage_msg, // In case of wrong options
                    argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(0));

    pid_t root_pid = getppid();

    //printf("parent pid: %d\n", root_pid);

    int p[2];
    int c1_rpipe;
    int c2_rpipe;
    int parent_wpipe = 1;
    char c1_name[64], c2_name[64];
    for (; depth > 0; depth--) { //An iterative approach to creating the hierarchy
        
        if (depth == 1) {//Immediately above leaves
            close(p[0]); //Use named pipe instead
            
            snprintf(c1_name, 64, "./pipe_%d_1", getpid());
            snprintf(c2_name, 64, "./pipe_%d_2", getpid());            
            if (mkfifo(c1_name, 0660) < 0) {
                perror("Named pipe creation error ");
                return -1;
            }
            if (mkfifo(c2_name, 0660) < 0) {
                perror("Named pipe creation error ");
                return -1;
            }
        } else { //Use unnamed pipe
            if (pipe(p) == -1) {
                perror("Pipe error ");
                return -1;
            } 
        }

        if (rand_range) {
            r_mid = r_rand(r_start, r_end - 1); //Random range
        } else {
            r_mid = (r_start + r_end)/2; //Half-half
        }

    	if (fork() != 0) { //If is parent, fork again
    		if (depth != 1) close(p[1]); //Parent closes writing end
    		c1_rpipe = p[0]; 

    		if (pipe(p) == -1) {
	    		perror("Pipe error ");
	    		return -1;
	    	} 
    		if (fork() != 0) { //If is parent, break loop after 2 forks
    			if (depth!=1) close(p[1]);
    			c2_rpipe = p[0];
    			break;
    		} else { //Child 2 closes reading end
                if (depth == 1) {
                    strncpy(parent_pipe_name, c2_name, 64);
                } else {
                    close(p[0]);
                    parent_wpipe = p[1];
                }
    			r_start = r_mid + 1; //Take second half of range
                id_range /= 2;
                id_start += id_range;
    		}
    	} else {
            if (depth == 1) {
                strncpy(parent_pipe_name, c1_name, 64);
            } else {
                close(p[0]); //Child 1 closes reading end
                parent_wpipe = p[1];
            }
    		r_end = r_mid; //Take first half of range
            id_range /= 2;
    	}
    }

    //Start timer
    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times(&tb1);

    FILE* parent_fp;
    if (parent_wpipe == 1 || depth == 0) {
        parent_fp = fopen(parent_pipe_name, "w"); //Top node opens named pipe
    } else {
        parent_fp = fdopen(parent_wpipe, "w");
    }

    if (depth == 0) { 
        char r_start_arg[16], r_end_arg[16], attr_num_arg[16], root_pid_arg[16], exec_name[64];
        snprintf(r_start_arg, 16, "%d", r_start);
        snprintf(r_end_arg, 16, "%d", r_end);
        snprintf(attr_num_arg, 16, "%d", attr_num);
        snprintf(root_pid_arg, 16, "%d", root_pid);

        int type = id_start%3;
        if (type == 0) { //Shell sort
            snprintf(exec_name, 64, "%s", "./shellsort");
        } else if (type == 1) { //Quicksort
            snprintf(exec_name, 64, "%s", "./quicksort");
        } else { //Bubble sort
            snprintf(exec_name, 64, "%s", "./bubblesort");
        }

        if (execlp(exec_name, exec_name, "-f", ifile, "-s", r_start_arg, "-e", r_end_arg,
         "-a", attr_num_arg, "-r", root_pid_arg, "-o", parent_pipe_name, NULL) < 0 ) {
            perror("Sorter exec fail ");
            return -1;
        }
    
    } else {
    	int nrecords = r_end - r_start + 1;
    	tax_rec* result_buf = (tax_rec*) malloc( (nrecords+2) * sizeof(tax_rec) ); //Buffer for merged result
    	tax_rec* c1_buf = (tax_rec*) malloc( (r_mid-r_start+1) * sizeof(tax_rec) ); 
    	tax_rec* c2_buf = result_buf + (r_mid-r_start + 1); //Leave enough space at start of array for merging
    	int c1_nread = 0 , c2_nread = 0;

        FILE *c1_fp, *c2_fp;
        if (depth == 1) {
            c1_fp = fopen(c1_name, "r");
            c2_fp = fopen(c2_name, "r");
        } else {
            c1_fp = fdopen(c1_rpipe, "r");
            c2_fp = fdopen(c2_rpipe, "r");
        }

    	
    	if (c1_fp==NULL || c2_fp==NULL) {
    		perror("Pipe stream error ");
    		close(c1_rpipe);
    		close(c2_rpipe);
    		return -1;
    	}
        int c1_i = 0, c2_i = 0;

        do{
        	c1_nread = fread(&c1_buf[c1_i], sizeof(tax_rec), (nrecords/2 + 1), c1_fp);
        	c2_nread = fread(&c2_buf[c2_i], sizeof(tax_rec), (nrecords/2 + 1), c2_fp);
            c1_i += c1_nread;
            c2_i += c2_nread;
        } while(c1_nread || c2_nread);

        tax_rec record;

    	fclose(c1_fp);
    	fclose(c2_fp);


        merge(c1_buf, c2_buf, result_buf, c1_i, c2_i, attr_num);

        if (parent_fp == NULL) {
            perror("Pipe stream error ");
            return -1;
        }

    	fwrite(result_buf, sizeof(tax_rec), (c1_i+c2_i), parent_fp);

        //Report time
        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
        (tb1.tms_utime + tb1.tms_stime));
        printf("Merging complete. Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
        (t2 - t1) / ticspersec, cpu_time / ticspersec);

        free(c1_buf);
        free(result_buf);
    	
        fclose(parent_fp);
        if (depth == 1) { //Nodes immediately above leaves clean up named pipes
            if(unlink(c1_name) < 0) {
                perror("Named pipe unlink error ");
            }
            if(unlink(c2_name) < 0) {
                perror("Named pipe unlink error ");
            }
        }
    }


    return 0;
}