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
    //char depth[56], attr_num[56];
    int attr_num, depth, rand_range = 0;
    int argNum = 0;
    int record_count = 0;
    char ofile[64], ifile[256];
    //char *fifo = "./myfifo";
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
        //TODO: take in number of records as argument
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
            if (sscanf(optarg, "%s", ofile) < 1) {
                printf("Invalid output file name\n");
                return -1;
            }
            output_fp = fopen(ofile, "w");
            if (output_fp==NULL) {
                //printf("%s\n", optarg);
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

        default: /* '?' */
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
/*
    int p[2];
    if (pipe(p) == -1) {
        perror("Root pipe error: ");
        return -1;
    }
*/
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

    if (fork() != 0) {
        //printf("I'm the root!\n");
        //close(p[1]); //Parent closes writing end

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
/*
        tax_rec record;
        for (int i=0; i<nread; i++){
            record = records[i];
            printf("id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
        }
*/
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

        printf("SIGUSR1 count: %d\n", sig_usr1_count);
        printf("SIGUSR2 count: %d\n", sig_usr2_count);
        printf("SIGALRM count: %d\n", sig_alrm_count);

    } else {
        /*
        close(p[0]); //Child closes reading end
        if(dup2(p[1], 1) < 0) { //Redirect sdout to pipe
            perror("Dup2 error: ");
            return -1;
        }*/

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
        //if (execlp("./node", "./node", "-d", "3", "-a", "0", NULL) == -1) perror("Exec failed ");
    }

    return 0;
}