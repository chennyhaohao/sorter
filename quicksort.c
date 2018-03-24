#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/times.h>
#include "./record.c"
#include "./sort_func.h"

int main(int argc, char **argv) {
	int opt;
	char file_name[256], parent_pipe_name[64];
	int r_start=0, r_end=0, argNum=0, attr_num;
	int pipe_name_set = 0, root_pid_set = 0;
	pid_t root_pid  = 1;
	char * usage_msg = 
	"Usage: %s [-f name of file to sort] [-o output file/pipe name] [-s range start] [-e range end] [-a AttrNum] [-r root_pid]\n";
	while ((opt = getopt(argc, argv, "a:f:o:s:e:r:")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) {
        case 'f':
            snprintf(file_name, 256, "%s", optarg);
            argNum++;
            break;

        case 's':
        	r_start = atoi(optarg);
        	argNum++;
        	break;

        case 'e':
        	r_end = atoi(optarg);
        	argNum++;
        	break;

        case 'r':
        	root_pid = atoi(optarg);
        	root_pid_set = 1;
        	break;

        case 'a':
        	attr_num = atoi(optarg);
        	argNum++;
        	break;

        case 'o':
            snprintf(parent_pipe_name, 64, "%s", optarg);
            pipe_name_set = 1;
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

	FILE *fp = fopen(file_name, "rb");
	FILE * parent_fp;
	if (pipe_name_set) {
		parent_fp = fopen(parent_pipe_name, "w");
	} else {
		parent_fp = fdopen(1, "w");
	}

	if (!fp || !parent_fp) {
		perror("File open failed ");
		return -1;
	}

	tax_rec * records;
	tax_rec record;

	int nrecords = r_end - r_start + 1;
	records = malloc(nrecords*sizeof(tax_rec));

    fseek(fp, r_start*sizeof(tax_rec), SEEK_SET);
	int nread = fread(records, sizeof(tax_rec), nrecords, fp);
    fclose(fp);

    //Start timer
    double t1, t2, cpu_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times(&tb1);

	quicksort(records, nread, attr_num);
	

    int nwrite = fwrite(records, sizeof(tax_rec), nread, parent_fp);
    printf("nwrite: %d\n", nwrite);

    //Report time
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
    (tb1.tms_utime + tb1.tms_stime));
    printf("Quick sort complete. Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
    (t2 - t1) / ticspersec, cpu_time / ticspersec);

	free(records);

    fclose(parent_fp);
    if (root_pid_set) {
    	int signum;
    	
    	signum = SIGUSR2;
    	
    	kill(root_pid, signum);
    	printf("Sent SIGUSR2 to root (%d)\n", root_pid);
	}
}