#include "types.h"
#include "stat.h"
#include "user.h"

void printLine() {
    int i;
    int sizeOfArray = 100;
    printf(1, "\n");
    for (i=0;i<sizeOfArray;i++){
        printf(2, "Child %d prints for the %d time.\n",getpid(),i);
    }
}

void rrtest(void) {
    int numberOfForks = 10;
    int wTime[numberOfForks];
    int rTime[numberOfForks];
    int forkId;

    int i;
    for (i=0;i<numberOfForks;i++){
        forkId = fork();
        if(forkId == 0) {
            printLine();
            exit();
        }
    }
    while(wait() > 0){
        printf(2, "PID : %d - Wait time : %d - Running time : %d - Turn Around time : %d.\n", getperformancedata(&wTime[i], &rTime[i]), wTime[i], rTime[i], rTime[i] + wTime[i]);
    }
}

int main(void) {
    rrtest();
    exit();
}
