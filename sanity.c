#include "types.h"
#include "stat.h"
#include "user.h"

void printLine() {
    int i;
    int sizeOfArray = 500;
    printf(1, "\n");
    for (i=0;i<sizeOfArray;i++){
        printf(2, "PID %d prints for the %d time.\n",getpid(),i);
    }
}

void sanitytest(void) {
    int numberOfForks = 30;
    int wTime;
    int rTime;
    int pid;
    int forkId;

    int i;
    for (i=0;i<numberOfForks;i++){
        forkId = fork();
        if(forkId == 0) {
            pid = getpid();
            if(pid %3 == 0){
                nice();
            }
            else if(pid %3 == 1){
                nice();
                nice();
            }
            else{
            }
            printLine();
            exit();
        }
    }
    while(wait() > 0){
        printf(2, "PID : %d - Wait time : %d - Running time : %d - Turn Around time : %d.\n", getperformancedata(&wTime, &rTime), wTime, rTime, rTime + wTime);
    }
}

int main(void) {
    sanitytest();
    exit();
}
