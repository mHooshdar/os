#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	int pid;

	pid = fork();
	if(pid > 0) {
        printf("parent: child=%d\n", pid);
        pid = wait(0);
        printf("child %d is done\n", pid);
	} else if(pid == 0) {
        printf("child: exiting\n");
        exit(0);
	} else {
	    printf("fork error\n");
	}

	return 0;
}
