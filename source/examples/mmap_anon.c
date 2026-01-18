#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define DATA_LEN 128 // bytes

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

int main(void)
{
    char *data = mmap(NULL, DATA_LEN, PROT_READ|PROT_WRITE,
                      MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    if (data == NULL) {
        perror("mmap");
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork");
            return 1;
                
        case 0:
            puts("child: sleeping");
            // Snooze so it's very likely the parent wins the race
            sleep(1);
            puts("child: reading");
            printf("child: %s\n", data);
            break;

        default:
            puts("parent: writing");
            strcpy(data, "Hello from shared memory!");
            puts("parent: waiting");
            wait(NULL);
            break;
    }

    munmap(data, 128);
}
