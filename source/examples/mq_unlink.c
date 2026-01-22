#ifdef __APPLE__
#warning "Apple doesn't support POSIX message queues."
int main(void) {}
#else

#include <stdio.h>
#include <mqueue.h>

int main(void)
{
    if (mq_unlink("/mq_test") == -1) {
        perror("/mq_test");
        return 1;
    }
}

#endif
