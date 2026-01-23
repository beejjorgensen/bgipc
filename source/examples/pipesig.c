#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <poll.h>

int pipefd[2];

void handler(int sig)
{
    (void)sig;
    write(pipefd[1], "1", 1);
}

void main_loop(void)
{
    char line[1024];

    struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    struct pollfd pollfds[2] = {
        { .fd=0, .events=POLLIN },
        { .fd=pipefd[0], .events=POLLIN },
    };

    puts("Enter lines of text, or \"quit\" to quit.");

    for (;;) {
        int st;

restart:
        st = poll(pollfds, 2, 0);
        if (st == -1 && errno == EINTR) goto restart;

        if (st > 0) {
            if ((pollfds[0].revents & POLLIN)) {
                if (fgets(line, sizeof line, stdin) == NULL)
                    return;

                int len = strlen(line);
                if (line[len-1] == '\n') line[len-1] = '\0';

                if (strcmp(line, "quit") == 0)
                    return;

                printf("You entered: \"%s\"\n", line);
            }

            else if ((pollfds[1].revents & POLLIN)) {
                char sigdata[1024];

                int count = read(pipefd[0], sigdata, sizeof sigdata);

                for (int i = 0; i < count; i++)
                    if (sigdata[i] == '1')
                        printf("SIGUSR1 occurred\n");
            }
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
