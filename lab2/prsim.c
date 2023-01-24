#include <stdio.h>
#include <stdlib.h>
#include  <stdbool.h> 
#include <string.h>
#include "queue.h"
#include <errno.h>

struct Process {
    char name[256];
    int time;
    float prob;
    int ioTime;
    int timeToBlock;
    bool willBlock;
    int totalCPU;
    int givenCPU;
    int blockedIO;
    int doingIO;
    int rr;
};

struct Process* makeProcess(char* name, int time, float prob) {
    struct Process* p = malloc(sizeof(struct Process));
    strcpy(p->name, name);
    p->time = time;
    p->prob = prob;
    p->timeToBlock = -1;
    p->totalCPU = 0;
    p->givenCPU = 0;
    p->blockedIO = 0;
    p->doingIO = 0;
    p->rr = 0;
    return p;
}

int len (char* str){
    int i = 0;
    while (str[i] != '\0'){
        i += 1;
    }

    return i;
}

queue_t readFile(char* fileName, FILE* fp, char* option, char* exe){
    /* bad option (not -f or -r) */
    if (option[1] != 'r' && option[1] != 'f'){
        fprintf(stderr, "Usage: %s [-r | -f] file\n", exe);
        exit(1);
    }      

    fp = fopen(fileName, "r");
    /* can't open file */
    if (fp == NULL){
        errno = ENOENT;
        perror("filename");
        exit(1);
    }

    queue_t q = queue_create();
    
    
    char name[256];
    int time;
    float prob;
    int curr = EOF;
    int curLine = 0;
    do
    {   
        curr = fscanf(fp, "%s %d %f", name, &time, &prob);
        curLine += 1;
        if (curr != -1) {
            /* the line in the input file is malformed */
            if (curr != 3){
                fprintf(stderr, "Malformed line %s(%d)\n", fileName, curLine);
                exit(1);
            }   
            /* process name is to long (over 10 characters) */
            if (len(name) > 10){
                fprintf(stderr, "name is too long %s(%d)\n", fileName, curLine);
                exit(1);
            }
            /* runtime is 0 or less */
            if (time <= 0){
                fprintf(stderr, "runtime is not positive integer %s(%d)\n", fileName, curLine);
                exit(1);
            }
            /* probability is not between 0 and 1 */
            if (prob < 0 || prob > 1){
                fprintf(stderr, "probability < 0 or > 1 %s(%d)\n", fileName, curLine);
                exit(1);
            }

            struct Process* p = makeProcess(name, time, prob);
            // printf("%s %d %f \n", p->name, p->time, p->prob);
            queue_enqueue(q, p);

        }
    } while (curr != EOF);

    fclose(fp);
    return q;
}

void checkBlock(struct Process* head, int time, char* option) {
    
    if (head->time < 2) {
        // printf("<%s> will not be block since time <2 \n", head->name);
        head->willBlock = false;
    }
    else{ 
        int r =(int) random();
        float b = (float) r / RAND_MAX;
        // printf("<%s> checking for block, using random number %d at %f \n", head->name, r, b);
        if (head->prob >= b && head->time > 0) {
            int rand = (int) random();
            
            int r;
            // FCFS
            if(option[1] == 'f'){
                r = ( rand % head->time) + 1;
            }
            // Round Robin
            else{
                if(head->time <= 5){
                    r = ( rand % head->time) + 1;
                }
                else{
                    r = ( rand % 5) + 1;
                }
            }
            
            // printf("\t\t\t\tCPU rand: %d\tr = %d\n", rand, r);
            head->timeToBlock = time + r -1 ;

            // printf("<%s> will block in %d\n", head->name, r);
            head->willBlock = true;
        }
    }
}



int main(int argc, char* argv[]){
    (void) srandom(12345);
    FILE* fp;
    char* option = argv[1];
    queue_t cpuQ = readFile(argv[2], fp, option, argv[0]);
    queue_t ioQ = queue_create();

    int numProc = queue_length(cpuQ);

    /*
    * printing process information
    */
    /* header line */
    printf("Processes:\n\n");
    printf("   name     CPU time  when done  cpu disp  i/o disp  i/o time\n");

    int cpuBusy = 0;
    int cpuIdle = 1;
    int cpuDispatches = 0;

    int ioBusy = 0;
    int ioIdle = 0;
    int ioDispatches = 0;



    int io_block=0;

    
    // printf("/--------------------------/\n");
    int time = 0;
    
    struct Process* CPU = NULL;
    struct Process* IO = NULL;
    while (queue_length(cpuQ) != 0 || queue_length(ioQ) != 0 || CPU != NULL || IO != NULL) {
        
        time++;
        //printf("/----------------------------CURRENT TICK %d----------------------------/\n", time);

        //printf("/-------/READY QUEUE FUNCTION/-------/\n");
        if (queue_length(cpuQ) > 0) {
            if (CPU == NULL) {
                struct Process* head = cpuQ->head->value;
                checkBlock(head, time, option);
                CPU = head;
                CPU->givenCPU++;
                CPU->rr = 0;
                // printf("<%s> HAS Entered CPU, TIME: %d\n", CPU->name, time);
                queue_delete(cpuQ, head);
                cpuDispatches++;
            }
        }
    

        //printf("/-------/CPU FUNCTION/---------------/\n");
        if (CPU != NULL) {
            //printf("CPU ACTIVE: <%s> TIME: %d\n", CPU->name, CPU->time);
            if (CPU->time == 0)  { 
                //printf("<%s> HAS FINISHED, TIME: %d\n", CPU->name, time);

                /* for process information */
                printf("%-10s %6d     %6d    %6d    %6d    %6d\n", CPU->name, CPU->totalCPU, time, CPU->givenCPU, CPU->blockedIO, CPU->doingIO);
                free(CPU);
                CPU = NULL;

                if (!(queue_length(cpuQ) == 0 && queue_length(ioQ) == 0 && IO == NULL)){
                    cpuIdle++;
                }
            } else {
                CPU->time--;
                CPU->rr++;
                CPU->totalCPU++;
                
                 //printf("PROCESSING <%s> , REMAINING TIME In CPU: %d\n", CPU->name, CPU->timeToBlock-time);
                 //printf("<%s> with process time %d, timeToBlock: %d, willBlock: %d \n,", CPU->name, CPU->time,CPU->timeToBlock,CPU->willBlock);
                
                if (CPU->timeToBlock == time && CPU->timeToBlock != 0 && CPU->willBlock){
                     //printf("<%s> EXITING CPU TO IO QUEUE, CPU TIME: %d\n", CPU->name, CPU->time);
                    CPU->timeToBlock = 0;
                    CPU->blockedIO++;
                    queue_enqueue(ioQ, CPU);
                    CPU = NULL;
                    io_block = 1;
                    //printf("io_block has set to be 1\n");

                } else if ((CPU->rr == 5 || CPU->time == 0) && option[1] == 'r' && !CPU->willBlock) {
                    //printf("QUANTA ENDED\n");
                    queue_enqueue(cpuQ, CPU);
                    CPU = NULL;
                    if (queue_length(cpuQ) > 0) {
                        if (CPU == NULL) {
                                struct Process* head = cpuQ->head->value;
                                checkBlock(head, time, option);
                                CPU = head;
                                CPU->givenCPU++;
                                CPU->rr = 0;
                                //printf("<%s> HAS Entered CPU, TIME: %d\n", CPU->name, time);
                                queue_delete(cpuQ, head);
                                cpuDispatches++;
                            }
                        }
                }
                cpuBusy++;
            }

        } else {
            cpuIdle++;
        }


        


        

        if(io_block==1 && IO == NULL && queue_length(ioQ) ==1){//situation with tick5, file2
            // printf("Didn't run because the process just enter the io queue at the same cpu cycle/\n");
            io_block=0;
            ioIdle++;
            // printf("io_block has set to be 0\n");
        }  
        else{
            io_block=0;
            // printf("/-------/IO QUEUE FUNCTION/----------/\n");
            if (queue_length(ioQ) > 0) {
                if (IO == NULL) {
                    struct Process* curr = ioQ->head->value;
                    int r = 0;
                    if(curr->time == 0){
                        r=1;
                    }
                    else if (curr->time > 0) {
                        int rand = (int) random();
                        r = (rand % 30) + 1;
                        // printf("\t\t\t\tI/O rand: %d\tr = %d\n", rand, r);
                    }
                    curr->ioTime = r;
                    // printf("|<%s>| ENTERING IO, ioTime: %d\n", curr->name, r);
                    IO = curr;
                    IO->doingIO += r;
                    queue_delete(ioQ, curr);
                    ioDispatches++;
                    
                }
            }    

            
            // printf("/-------/IO FUNCTION/----------------/\n");
            if (IO != NULL) {
                IO->ioTime--;
                // printf("IO ACTIVE, CURRENT PROCESS <%s> Remaining ioTIME: %d\n", IO->name, IO->ioTime);
                if (IO->ioTime <= 0) {
                    // printf("|<%s>| EXITING IO, entering CPU\n", IO->name);
                    queue_enqueue(cpuQ, IO);
                    IO = NULL;
                    }
                ioBusy++;
            } else {
                ioIdle++;
                // printf("io idle +1\n");
            }
            
            
        }  
        
        // printf("\n");
        
    }

    // printf("System:\nThe wall clock time at which the simulation finished: %d\n", time);
    // printf("CPU:\nTotal time spent busy: %d\nTotal time spent idle: %d\nNumber of dispatches: %d\n", cpuBusy, cpuIdle, cpuDispatches);
    // printf("IO:\nTotal time spent busy: %d\nTotal time spent idle: %d\nNumber of dispatches: %d\n", ioBusy, ioIdle, ioDispatches);
    
    double cpuUtil = (double)cpuBusy / (double)time; 
    double ioUtil = (double)ioBusy / (double)time; 
    double throughPut = (double)numProc / (double)time;

    
    /* print clock time at end */
        printf("\nSystem:\n");
        printf("The wall clock time at which the simulation finished: %d\n", time);

        /* print cpu statistics */
        printf("\nCPU:\n");
        printf("Total time spent busy: %d\n", cpuBusy);
        printf("Total time spent idle: %d\n", cpuIdle);
        printf("CPU utilization: %.2f\n", cpuUtil);
        printf("Number of dispatches: %d\n", cpuDispatches);
        printf("Overall throughput: %.2f\n", throughPut);

        /* print i/o statistics */
        printf("\nI/O device:\n");
        printf("Total time spent busy: %d\n", ioBusy);
        printf("Total time spent idle: %d\n", ioIdle);
        printf("I/O utilization: %.2f\n", ioUtil);
        printf("Number of dispatches: %d\n", ioDispatches);
        printf("Overall throughput: %.2f\n", throughPut);
    
    return 0;
}