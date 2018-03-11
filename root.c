#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "./record.c"

#define FIFO "./root_pipe"

int main(int argc, char **argv) 
{
	int opt;
    //char depth[56], attr_num[56];
    char buf[1024];
    int attr_num, depth;
    int argNum = 0;
    int r_start = 0, r_end = 10;
    char *ofile;
    //char *fifo = "./myfifo";
    
    FILE * output_fp = fdopen(1, "w");
    if (output_fp==NULL) {
        perror("Output file creation error: ");
        return -1;
    }

    char * usage_msg = "Usage: %s [-d Depth of binary tree] [-a Attribute Number]\n";
    while ((opt = getopt(argc, argv, "d:a:o::")) != -1) { // Use getopt to parse commandline arguments
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
            ofile = optarg;
            output_fp = fopen("b.out", "w");
            if (output_fp==NULL) {
                //printf("%s\n", optarg);
                perror("Invalid output file name");
                return -1;
            }
            break;

        default: /* '?' */
            fprintf(stderr, usage_msg, // In case of wrong options
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (argNum < 2) {
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
    if(mkfifo(FIFO, 0660) < 0) { //Create named pipe
        perror("Fifo creation error ");
        return -1;
    }


    if (fork() != 0) {
        //printf("I'm the root!\n");
        //close(p[1]); //Parent closes writing end
        
        tax_rec records[999];
        FILE* input_fp = fopen(FIFO, "r"); //Open read end of named pipe
        int nread = fread(records, sizeof(tax_rec), 999, input_fp);
        printf("Records read: %d\n", nread);

        tax_rec record;
        for (int i=0; i<nread; i++){
            record = records[i];
            printf("id: %d, name: %s %s, income: %f\n", record.id, record.fname, record.lname, record.income);
        }

        int nwrite = fwrite(records, sizeof(tax_rec), nread, output_fp);
        printf("Records written: %d\n", nwrite);

        //close(p[0]);
        fclose(input_fp);
        fclose(output_fp);
        printf("Look at me! I'm the root!\n");
        if(unlink(FIFO) < 0) {
            perror("Named pipe unlink error ");
        }
    } else {
        /*
        close(p[0]); //Child closes reading end
        if(dup2(p[1], 1) < 0) { //Redirect sdout to pipe
            perror("Dup2 error: ");
            return -1;
        }*/

        char depth_arg[64], attr_num_arg[64];
        sprintf(depth_arg, "%d", depth);
        sprintf(attr_num_arg, "%d", attr_num);
        //if (execlp("./node", "./node", "-d", depth, "-a", attr_num_arg, NULL) == -1) perror("Exec failed ");
        if (execlp("./node", "./node", "-d", "3", "-a", "0", NULL) == -1) perror("Exec failed ");
    }

    return 0;
}