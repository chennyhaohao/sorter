#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include "./record.c"


int sig_usr1_count = 0;
int sig_usr2_count = 0;
int sig_alrm_count = 0;

void sig_handler(int signum) {
    signal(SIGALRM, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    if (signum == SIGUSR2) {
        sig_usr2_count++;
        printf("QS sorter signal received.\n");
    } else if (signum == SIGUSR1) {
        sig_usr1_count++;
        printf("SH sorter signal received.\n");
    } else if (signum == SIGALRM) {
        sig_alrm_count++;
        printf("BS sorter signal received.\n");
    }
}

int main(int argc, char **argv) 
{
	int opt;
    int attr_num, depth, rand_range = 0;
    int argNum = 0;
    int record_count = 0;
    char ofile[64], ifile[256];
    int total_expected, usr1_expected, usr2_expected, alrm_expected;
    //Set signal handlers
    if(signal(SIGALRM, sig_handler) == SIG_ERR) perror("Cannot catch SIGALRM ");
    if(signal(SIGUSR1, sig_handler) == SIG_ERR) perror("Cannot catch SIGUSR1 ");    
    if(signal(SIGUSR2, sig_handler) == SIG_ERR) perror("Cannot catch SIGUSR2 ");

    FILE * output_fp = fdopen(1, "w"), *target_fp;
    if (output_fp==NULL) {
        perror("Output file creation error: ");
        return -1;
    }

    char * usage_msg = "Usage: %s [-d Depth of binary tree] [-a Attribute Number] [-f File to sort] [-r if each sorter takes randomized range]\n";
    while ((opt = getopt(argc, argv, "d:a:o:f:r")) != -1) { // Use getopt to parse commandline arguments
        switch (opt) {
        case 'd':
            depth = atoi(optarg);
            if (depth < 0 || depth > 6) {
                printf("Invalid Depth.\n");
                return -1;
            }
            argNum++;
            total_expected = (int) (pow(2, depth) + 0.5);
            usr1_expected = (total_expected + 2)/3;
            usr2_expected = (total_expected + 1)/3;
            alrm_expected = total_expected/3;
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
            if (sscanf(optarg, "%s", ofile) < 1) {
                printf("Invalid output file name\n");
                return -1;
            }
            output_fp = fopen(ofile, "w");
            if (output_fp==NULL) {
                perror("Invalid output file name");
                return -1;
            }
            break;

        case 'f':
            if (sscanf(optarg, "%s", ifile) < 1) {
                printf("Invalid input file name\n");
                return -1;
            }
            target_fp = fopen(ifile, "r");
            if (!target_fp) {
                perror("Invalid input file name");
                return -1;
            }
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

    if (argNum < 3) {
        fprintf(stderr, usage_msg, // In case of wrong options
                    argv[0]);
        exit(EXIT_FAILURE);
    }

    tax_rec buf[100];
    int nread;
    do {
        nread = fread(buf, sizeof(tax_rec), 100, target_fp);
        record_count += nread;
    } while (nread > 0);
    printf("Record count: %d\n", record_count);
    fclose(target_fp);


    char pipe_name[64];
    snprintf(pipe_name, 64, "./pipe_%d", getpid());
    if(mkfifo(pipe_name, 0660) < 0) { //Create named pipe
        perror("Fifo creation error ");
        return -1;
    }

    

    printf("root pid: %d\n", getpid());

    if (fork() != 0) { //Parent reads results from child and writes to output
        
        //Start timer
        double t1, t2, cpu_time;
        struct tms tb1, tb2;
        double ticspersec;
        ticspersec = (double) sysconf(_SC_CLK_TCK);
        t1 = (double) times(&tb1);

        tax_rec* records = malloc(record_count * sizeof(tax_rec));
        if (!records) {
            perror("Malloc failed");
            return -1;
        }
        FILE* input_fp = fopen(pipe_name, "r"); //Open read end of named pipe
        nread = fread(records, sizeof(tax_rec), record_count, input_fp);
        printf("Records read: %d\n", nread);

        int nwrite = fwrite(records, sizeof(tax_rec), nread, output_fp);
        printf("Records written: %d\n", nwrite);

        //Clean up: free resources
        free(records);
        fclose(input_fp);
        fclose(output_fp);

        printf("Look at me! I'm the root!\n");

        if(unlink(pipe_name) < 0) {
            perror("Named pipe unlink error ");
        }

        //Report time
        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
        (tb1.tms_utime + tb1.tms_stime));
        printf("Root complete. Run time (turnaround time) was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
        (t2 - t1) / ticspersec, cpu_time / ticspersec);

        printf("SIGUSR1 received: %d\n", sig_usr1_count);
        printf("SIGUSR2 received: %d\n", sig_usr2_count);
        printf("SIGALRM received: %d\n", sig_alrm_count);

        int total_missing = total_expected - sig_usr1_count - sig_usr2_count -sig_alrm_count;
        int usr1_missing = usr1_expected - sig_usr1_count;
        int usr2_missing = usr2_expected - sig_usr2_count;
        int alrm_missing = alrm_expected - sig_alrm_count;

        printf("Signal missing: %d (SIGUSR1: %d, SIGUSR2: %d, SIGALRM: %d)\n", total_missing, usr1_missing, 
            usr2_missing, alrm_missing);

    } else { //Child executes node executable
        
        char depth_arg[64], attr_num_arg[64], record_count_arg[64];
        snprintf(depth_arg, 64, "%d", depth);
        snprintf(attr_num_arg, 64, "%d", attr_num);
        snprintf(record_count_arg, 64, "%d", record_count);
        if (!rand_range) {
            if (execlp("./node", "./node", "-d", depth_arg, "-a", attr_num_arg, "-o", pipe_name,
            "-f", ifile, "-n", record_count_arg, NULL) == -1) perror("Exec failed ");
        } else { //Randomized range
            if (execlp("./node", "./node", "-d", depth_arg, "-a", attr_num_arg, "-o", pipe_name,
            "-f", ifile, "-n", record_count_arg, "-r", NULL) == -1) perror("Exec failed ");
        }
    }

    return 0;
}