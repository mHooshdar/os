#include "types.h"
#include "stat.h"
#include "user.h"

void printLine() {
    int i;
    int sizeOfArray = 1000;
    printf(1, "\n");
    for (i=0;i<sizeOfArray;i++){
        printf(2, "Child %d prints for the %d time.\n",getpid(),i);
    }
}

void rrtest(void) {
    int numberOfForks = 10;
    int wTime;
    int rTime;
    int pid;
    int forkId;

    int i;
    for (i=0;i<numberOfForks;i++){
        forkId = fork();
        if(forkId == 0) {
            pid = getpid();
            printLine();
            getperformancedata(&wTime, &rTime);
            printf(2, "PID : %d - Wait time : %d - Running time : %d - Turn Around time : %d.\n", pid, wTime, rTime, rTime + wTime);
            exit();
        }
    }
    wait();
}

int main(void) {
    rrtest();
    exit();
}
