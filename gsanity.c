#include "types.h"
#include "stat.h"
#include "user.h"

void printLine() {
    int i;
    int sizeOfArray = 50;
    printf(1, "\n");
    for (i=0;i<sizeOfArray;i++){
        printf(2, "process %d is printing for the %d time.\n", getpid(), i);
    }
}

void mygsanity(void) {
    printf(2, "Father pid is %d.\n", getpid());
    //sleep(600);
    sleep(200);
    int forkId;
    forkId = fork();
    if(forkId == 0) {
        printLine();
        exit();
    }
    printLine();
}

int main(void) {
    mygsanity();
    exit();
}
