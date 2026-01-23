#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(void)
{
    sigset_t mask, oldmask;

    // Make a set with SIGINT in it
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    // Block everything in that set
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    // SIGINT is blocked for now!
    puts("Try to ^C out of here! You can't for 5 seconds!");
    sleep(5);

    // Back to how it was before, presumably without blocking SIGINT
    puts("Ok, now you can.");
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    puts("If you hit ^C, this won't print.");
}
