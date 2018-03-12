#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include "./record.c"
#include "./merge.h"

#define BUF_SIZE 4096

int main(int argc, char **argv) 
{	
	int attr_num, depth;
	int argNum = 0;
	int r_start = 0, r_end;
	int opt;
	char * usage_msg = "Usage: %s [-d Depth of binary tree] [-a Attribute Number] [-f file to sort] [-n number of records]\n";
    char parent_pipe_name[64], ifile[256];
    while ((opt = getopt(argc, argv, "d:a:o:f:n:")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) { 
        case 'd':
            depth = atoi(optarg);
            if (depth < 0 || depth > 6) {
                printf("Invalid Depth.\n");
                return -1;
            }
            argNum++;
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

        default: /* '?' */
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

    pid_t root_pid = getppid();

    printf("parent pid: %d\n", root_pid);

    int p[2];
    int c1_rpipe;
    int c2_rpipe;
    int parent_wpipe = 1;
    char c1_name[64], c2_name[64];
    for (; depth > 0; depth--) { //An iterative approach to creating the hierarchy
        if (pipe(p) == -1) {
            perror("Pipe error ");
            return -1;
        } 
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
        }

    	if (fork() != 0) { //If is parent, fork again
    		close(p[1]); //Parent closes writing end
    		c1_rpipe = p[0]; 

    		if (pipe(p) == -1) {
	    		perror("Pipe error ");
	    		return -1;
	    	} 
    		if (fork() != 0) { //If is parent, break loop after 2 forks
    			close(p[1]);
    			c2_rpipe = p[0];
    			break;
    		} else { //Child 2 closes reading end
                if (depth == 1) {
                    strncpy(parent_pipe_name, c2_name, 64);
                }
    			close(p[0]);
    			r_start = (r_start + r_end)/2 + 1; //Take second half of range
    			parent_wpipe = p[1];
    		}
    	} else {
            if (depth == 1) {
                strncpy(parent_pipe_name, c1_name, 64);
            }
    		close(p[0]); //Child 1 closes reading end
    		r_end = (r_start + r_end)/2; //Take first half of range
    		parent_wpipe = p[1];
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
        char r_start_arg[16], r_end_arg[16], attr_num_arg[16], root_pid_arg[16];
        snprintf(r_start_arg, 16, "%d", r_start);
        snprintf(r_end_arg, 16, "%d", r_end);
        snprintf(attr_num_arg, 16, "%d", attr_num);
        snprintf(root_pid_arg, 16, "%d", root_pid);

        if (execlp("./sorter", "./sorter", "-f", ifile, "-s", r_start_arg, "-e", r_end_arg,
         "-a", attr_num_arg, "-r", root_pid_arg, "-o", parent_pipe_name , "-m", "0", NULL) < 0 ) {
            perror("Sorter exec fail ");
            return -1;
        }
    /*//Sorter code
    	FILE *fp = fopen("test_data/test_data_100000.bin", "rb");
		tax_rec * records;
		tax_rec record;
		if (!fp) {
			perror("File open failed: ");
			close(parent_wpipe);
			return -1;
		}

		int nrecords = r_end - r_start + 1;
		records = malloc(nrecords*sizeof(tax_rec));

        fseek(fp, r_start*sizeof(tax_rec), SEEK_SET);
		int nread = fread(records, sizeof(tax_rec), nrecords, fp);
        fclose(fp);

		//quicksort(records, nread, attr_num);
        shellsort(records, nread, attr_num);

        //FILE *parent_fp = fdopen(parent_wpipe, "w");

	   	for (int i=0; i<nread; i++){
			record = records[i];
			printf("id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
		}
        printf("\n");

        int nwrite = fwrite(records, sizeof(tax_rec), nread, parent_fp);
        //printf("nwrite: %d\n", nwrite);

        //Report time
        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
        (tb1.tms_utime + tb1.tms_stime));
        printf("Quick sort complete. Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
        (t2 - t1) / ticspersec, cpu_time / ticspersec);

		free(records);


    	//char * msg = "blahblahblahblahblah\n";
    	//write(parent_wpipe, msg, strlen(msg));
    	//msg = "I'm a sorter!\n";
    	//write(1, msg, strlen(msg));
    	//close(parent_wpipe);
        fclose(parent_fp);
        kill(root_pid, SIGUSR2);
        printf("Sent sig to %d\n", root_pid);*/
    } else {
    	int nrecords = r_end - r_start + 1;
    	//char msg[BUF_SIZE];
    	//snprintf(msg, 256, "I'm a merger! %d\n", depth);
    	tax_rec* result_buf = (tax_rec*) malloc( (nrecords+2) * sizeof(tax_rec) ); //Buffer for merged result
    	tax_rec* c1_buf = (tax_rec*) malloc( (nrecords/2 + 1) * sizeof(tax_rec) ); 
    	tax_rec* c2_buf = result_buf + (nrecords/2 + 1); 
    	//char c2_buf[BUF_SIZE];
    	int c1_nread = 0 , c2_nread = 0;

    	/*
    	do {
    		c1_nread = read(c1_rpipe, c1_buf, BUF_SIZE);
    		c2_nread = read(c2_rpipe, c2_buf, BUF_SIZE);
    		//write(parent_wpipe, c1_buf, c1_read);
    		//write(parent_wpipe, c2_buf, c2_read);
    	} while(c1_nread || c2_nread);

        printf("c1 read: %d\n", c1_nread);
        printf("c2 read: %d\n", c2_nread);
    	*/
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
/*        for (int i=0; i<c1_i; i++){
            record = c1_buf[i];
            printf("child 1: id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
        }

        for (int i=0; i<c2_i; i++){
            record = c2_buf[i];
            printf("child 2: id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
        }
*/

        
        //printf("nrecords: %d\n", nrecords);
        //printf("depth: %d  ", depth);
        //printf("c1 read: %d\n", c1_i);
        //printf("c2 read: %d\n", c2_i);


    	//snprintf(msg, 256, "I'm a merger! depth: %d c1: %d c2: %d\n", depth, c1_read, c2_read);
	    //write(1, msg, strlen(msg));
    	fclose(c1_fp);
    	fclose(c2_fp);


        merge(c1_buf, c2_buf, result_buf, c1_i, c2_i, attr_num);

        if (parent_fp == NULL) {
            perror("Pipe stream error ");
            return -1;
        }
/*
        for (int i=0; i<c1_i+c2_i; i++){
            record = result_buf[i];
            printf("id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
        }
*/
    	fwrite(result_buf, sizeof(tax_rec), (c1_i+c2_i), parent_fp);

        //Report time
        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
        (tb1.tms_utime + tb1.tms_stime));
        printf("Merging complete. Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
        (t2 - t1) / ticspersec, cpu_time / ticspersec);

        free(c1_buf);
        free(result_buf);
    	//write(parent_wpipe, c2_buf, c2_read);
    	//close(parent_wpipe);
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