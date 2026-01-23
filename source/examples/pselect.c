#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

volatile sig_atomic_t sigusr1_happened;

void handler(int sig)
{
    (void)sig;
    sigusr1_happened = 1;
}

void main_loop(void)
{
    char line[1024];
    fd_set readfds;

    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = 0
    };
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return;
    }

    puts("Enter lines of text, or \"quit\" to quit.");

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    for (;;) {
        int st;

        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        st = pselect(1, &readfds, NULL, NULL, NULL, &oldmask);

        if (st == -1 && errno == EINTR) {
            if (sigusr1_happened) {
                sigusr1_happened = 0;
                printf("SIGUSR1 occurred\n");
            }

        } else if (st > 0 && FD_ISSET(0, &readfds)) {
            if (fgets(line, sizeof line, stdin) == NULL)
                return;

            int len = strlen(line);
            if (line[len-1] == '\n') line[len-1] = '\0';

            if (strcmp(line, "quit") == 0)
                return;

            printf("You entered: \"%s\"\n", line);
        }
    }
}

void sigusr1_pinger(pid_t pid)
{
    for (;;) {
        sleep(3);
        if (kill(pid, SIGUSR1) == -1)
            _exit(0);
    }
}

int main(void)
{
    pid_t child;

    switch(child = fork()) {
        case -1:
            perror("fork");
            return 1;

        case 0:
            sigusr1_pinger(getppid());
            return 0;

        default:
            main_loop();
            break;
    }

    puts("Quitting, sending SIGTERM to child");

    kill(child, SIGTERM);
    wait(NULL);
}
