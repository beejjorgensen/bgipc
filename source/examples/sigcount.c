#include <stdio.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t count;

void handler(int sig)
{
    (void)sig;

    count = -1235;
}

void increment(void)
{
    int next_count = count + 1;

    printf("Count is %d, next should be %d\n", count, next_count);

    // Sleep to slow down time to demo the problem
    sleep(2);
    count++;

    if (count == next_count)
        puts("Everything is swell!");
    else
        printf("%d != %d! Aaa! ERROR DOES NOT COMPUTE!\n", count,
            next_count);
}

int main(void)
{
    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    if (fork() == 0) {
        sleep(1);
        kill(getppid(), SIGUSR1);
    } else {
        increment();
    }
}
