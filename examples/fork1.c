/*
** fork1.c -- demonstrates usage of fork() and wait()
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
	pid_t pid;
	int rv;

	switch(pid = fork()) {
	case -1:
		perror("fork");  /* something went wrong */
		exit(1);		 /* parent exits */

	case 0:
		printf(" CHILD: This is the child process!\n");
		printf(" CHILD: My PID is %d\n", getpid());
		printf(" CHILD: My parent's PID is %d\n", getppid());
		printf(" CHILD: Enter my exit status (make it small): ");
		scanf(" %d", &rv);
		printf(" CHILD: I'm outta here!\n");
		exit(rv);

	default:
		printf("PARENT: This is the parent process!\n");
		printf("PARENT: My PID is %d\n", getpid());
		printf("PARENT: My child's PID is %d\n", pid);
		printf("PARENT: I'm now waiting for my child to exit()...\n");
		wait(&rv);
		printf("PARENT: My child's exit status is: %d\n", WEXITSTATUS(rv));
		printf("PARENT: I'm outta here!\n");
	}

	return 0;
}

