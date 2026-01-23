#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define RENTRANCY_ISSUE  // Define to see issues

void handler(int sig)
{
    (void)sig;

    char x[] = "Hello, world!";
    char *token;
    
#ifdef RENTRANCY_ISSUE
    if ((token = strtok(x, " ")) != NULL) do {
        write(1, "In handler: ", 12);
        write(1, token, strlen(token));
        write(1, "\n", 1);
    } while ((token = strtok(NULL, " ")) != NULL);

#else
    char *lasts;

    if ((token = strtok_r(x, " ", &lasts)) != NULL) do {
        write(1, "In handler: ", 12);
        write(1, token, strlen(token));
        write(1, "\n", 1);
    } while ((token = strtok_r(NULL, " ", &lasts)) != NULL);
#endif
}

void tokenizer(void)
{
    char s[] = "The quick brown fox jumped over the lazy dogs";

    char *token;
    
#ifdef RENTRANCY_ISSUE
    if ((token = strtok(s, " ")) != NULL) do {
        printf("In main: %s\n", token);
        // Sleep to slow down time to demo the problem
        sleep(1);
    } while ((token = strtok(NULL, " ")) != NULL);

#else
    char *lasts;

    if ((token = strtok_r(s, " ", &lasts)) != NULL) do {
        printf("In main: %s\n", token);
        // Sleep to slow down time to demo the problem
        sleep(1);
    } while ((token = strtok_r(NULL, " ", &lasts)) != NULL);

#endif

    puts("Done tokenizing");
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
        sleep(2);
        kill(getppid(), SIGUSR1);
    } else {
        tokenizer();
    }
}
