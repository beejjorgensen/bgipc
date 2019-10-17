/*
** pipe3.c -- a smartest pipe example
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	int pfds[2];

	pipe(pfds);

	if (!fork()) {
		dup2(pfds[1], 1); /* combines close() and dup(), atomically */
		close(pfds[0]); /* we don't need this */
		close(pfds[1]); /* we don't need this */
		execlp("ls", "ls", NULL);
	} else {
		dup2(pfds[0], 0); /* combines close() and dup(), atomically */
		close(pfds[0]); /* we don't need this */
		close(pfds[1]); /* we don't need this */
		execlp("wc", "wc", "-l", NULL);
	}

	return 0;
}
