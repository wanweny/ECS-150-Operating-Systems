#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

// Used Figure 5.3 from the book as a reference

void sigHandler(int signum){
    exit(0);
}

int main(int argc, char* argv[], char* envp[]){
    // less than 3 arg 
    if (argc < 3){
        printf("Error: Incorrect number of arguments\n");
        exit(1);
    } 

    // s is not a number
    bool sIsDigit = true;
    for (int i = 0; i < strlen(argv[1]); i++){
        if(!isdigit(argv[1][i])){
            sIsDigit = false;
            break;
        }
    }
    
    if(!sIsDigit){
        printf("Error: Invalid type for seconds\n");
        exit(1);
    }

    int s = atoi(argv[1]);

    // s is negative
    if (s < 0){
        printf("Error: Seconds cannot be negative\n");
        exit(1);
    }

    int forkVal = fork();

    if (forkVal < 0){ // fork failed
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (forkVal == 0){ //child process
        execve(argv[2], argv+2, envp);
        perror("execve");
    }
    else { //parent process
        //kill process after s seconds
        signal(SIGCHLD,sigHandler);
        sleep(s);
        // system alarm: set up with s, return sigalarm in s second
        kill(forkVal, SIGTERM); 
        wait(NULL);
        
    }
    return 1;

} 