#include "types.h"
#include "stat.h"
#include "user.h"

void printLine() {
    int i;
    int sizeOfArray = 1000;
    //printf(1, "\n")
    for (i=0;i<sizeOfArray;i++){
//        printf(2, "Child %d prints for the %d time.\n", getpid(), i);
    }
}

void myfrrtest(void) {
    int numberOfForks = 10;
    int forkId;
    printValid();
    int i;
    for (i=0;i<numberOfForks;i++){
        forkId = fork();
        if(forkId == 0) {
            printLine();
            exit();
        }
    }
    while(wait() > 0){

    }
    printValid();
}

int main(void) {
    myfrrtest();
    exit();
}
