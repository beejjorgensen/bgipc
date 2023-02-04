<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Semaphores -->
<!-- ======================================================= -->

# Semaphores {#semaphores}

Remember [file locking](#flocking)? Well, semaphores can be thought of
as really generic advisory locking mechanisms. You can use them to
control access to files, [shared memory](#shm), and, well, just about
anything you want. The basic functionality of a semaphore is that you
can either set it, check it, or wait until it clears then set it
("test-n-set"). No matter how complex the stuff that follows gets,
remember those three operations.

This document will provide an overview of semaphore functionality, and
will end with a program that uses semaphores to control access to a
file. (This task, admittedly, could easily be handled with file locking,
but it makes a good example since it's easier to wrap your head around
than, say, shared memory.)

<!-- ======================================================= -->
<!-- Grabbing some semaphores -->
<!-- ======================================================= -->

## Grabbing some semaphores

With System V IPC, you don't grab single semaphores; you grab _sets_ of
semaphores. You can, of course, grab a semaphore set that only has one
semaphore in it, but the point is you can have a whole slew of
semaphores just by creating a single semaphore set.

How do you create the semaphore set? It's done with a call to
`semget()`, which returns the semaphore id (hereafter referred to as the
`semid`):


``` {.c}
#include <sys/sem.h>

int semget(key_t key, int nsems, int semflg);
```

What's the `key`? It's a unique identifier that is used by different
processes to identify this semaphore set. (This `key` will be generated
using `ftok()`, described in the [Message Queues section](#mqftok).)

The next argument, `nsems`, is (you guessed it!) the number of
semaphores in this semaphore set. The maximum number is system
dependent, but it's probably around 32000. If you're needing more
(greedy wretch!), just get another semaphore set. You may pass `0` if
you're connecting to an existing semaphore set, but you must specify a
positive number if you're creating a new semaohore set.

Finally, there's the `semflg` argument. This tells `semget()` what the
permissions should be on the new semaphore set, whether you're creating
a new set or just want to connect to an existing one, and other things
that you can look up. For creating a new set, permissions can be
bitwise-OR'd with `IPC_CREAT`.

Here's an example call that generates the `key` with `ftok()` and
creates a 10 semaphore set, with 666 (`rw-rw-rw-`) permissions:

``` {.c}
#include <sys/ipc.h>
#include <sys/sem.h>

key_t key;
int semid;

key = ftok("/home/beej/somefile", 'E');
semid = semget(key, 10, 0666 | IPC_CREAT);
```

Congrats! You've created a new semaphore set! After running the program
you can check it out with the `ipcs` command. (Don't forget to remove it
when you're done with it with `ipcrm`!)

Wait! Warning! _¡Advertencia! ¡No pongas las manos en la tolva!_ (That's
the only Spanish I learned while working at Pizza Hut in 1990.  It was
printed on the dough roller.) Look here:

When you first create some semaphores, they're all uninitialized; it
takes another call to mark them as free (namely to `semop()` or
`semctl()`---see the following sections.) What does this mean? Well, it
means that creation of a semaphore is not _atomic_ (in other words, it's
not a one-step process). If two processes are trying to create,
initialize, and use a semaphore at the same time, a race condition might
develop.

One way to get around this difficulty is by having a single init process
that creates and initializes the semaphore long before the main
processes begin to run. The main process just accesses it, but never
creates nor destroys it.

Stevens refers to this problem as the semaphore's "fatal flaw". He
solves it by creating the semaphore set with the `IPC_EXCL` flag. If
process 1 creates it first, process 2 will return an error on the call
(with `errno` set to `EEXIST`.)  At that point, process 2 will have to
wait until the semaphore is initialized by process 1. How can it tell?
Turns out, it can repeatedly call `semctl()` with the `IPC_STAT` flag,
and look at the `sem_otime` member of the returned `struct semid_ds`
structure. If that's non-zero, it means process 1 has performed an
operation on the semaphore with `semop()`, presumably to initialize it.

For an example of this, see the demonstration program
[flx[`semdemo.c`|semdemo.c]], below, in which I generally reimplement
[Stevens's code](http://www.kohala.com/start/unpv22e/unpv22e.html).

In the meantime, let's hop to the next section and take a look at how to
initialize our freshly-minted semaphores.

<!-- ======================================================= -->
<!-- Controlling semaphores -->
<!-- ======================================================= -->

## Controlling your semaphores with `semctl()`

Once you have created your semaphore sets, you have to initialize them
to a positive value to show that the resource is available to use. The
function `semctl()` allows you to do atomic value changes to individual
semaphores or complete sets of semaphores.

``` {.c}
int semctl(int semid, int semnum, int cmd, ... /*arg*/);
```

`semid` is the semaphore set id that you get from your call to
`semget()`, earlier. `semnum` is the ID of the semaphore that you wish
to manipulate the value of. `cmd` is what you wish to do with the
semaphore in question. The last "argument", "`arg`", if required, needs
to be a `union semun`, which will be defined by you in your code to be
one of these:

``` {.c}
union semun {
    int val;               /* used for SETVAL only */
    struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
    ushort *array;         /* used for GETALL and SETALL */
};
```

(Note that `union semun` is now defined in the header files of modern
Linux systems. However, I don't know what feature test macro to use to
determine this, so only define this union if your system doesn't
already. Read the docs for `semctl()` for more information.)

The various fields in the `union semun` are used depending on the value
of the `cmd` parameter to `semctl()` (a partial list follows---see your
local man page for more):

|`cmd`|Effect|
|:--------:|----------------------------------------------------------|
|`SETVAL`|Set the value of the specified semaphore to the value in the `val` member of the passed-in `union semun`.|
|`GETVAL`|Return the value of the given semaphore.|
|`SETALL`|Set the values of all the semaphores in the set to the values in the array pointed to by the `array` member of the passed-in `union semun`. The `semnum` parameter to `semctl()` isn't used.<|
|`GETALL`|Gets the values of all the semaphores in the set and stores them in the array pointed to by the `array` member of the passed-in `union semun`. The `semnum` parameter to `semctl()` isn't used.|
|`IPC_RMID`|Remove the specified semaphore set from the system. The `semnum` parameter is ignored.|
|`IPC_STAT`|Load status information about the semaphore set into the `struct semid_ds` structure pointed to by the `buf` member of the `union semun`.|

For the curious, here are the (abbreviated) contents of the `struct
semid_ds` that is used in the `union semun`:

``` {.c}
struct semid_ds {
    struct ipc_perm sem_perm;  /* Ownership and permissions
    time_t          sem_otime; /* Last semop time */
    time_t          sem_ctime; /* Last change time */
    unsigned short  sem_nsems; /* No. of semaphores in set */
};
```

We'll use that `sem_otime` member later on when we write our `initsem()`
in the sample code, below.

<!-- ======================================================= -->
<!-- semop(): Atomic power! -->
<!-- ======================================================= -->

## `semop()`: Atomic power!

All operations that set, get, or test-n-set a semaphore use the
`semop()` system call. This system call is general purpose, and its
functionality is dictated by a structure that is passed to it, `struct
sembuf`:

``` {.c}
/* Warning! Members might not be in this order! */

struct sembuf {
    ushort sem_num;
    short sem_op;
    short sem_flg;
};
```

Of course, `sem_num` is the number of the semaphore in the set that you
want to manipulate. Then, `sem_op` is what you want to do with that
semaphore. This takes on different meanings, depending on whether
`sem_op` is positive, negative, or zero, as shown in the following
table:

|`sem_op`|What happens|
|:------:|--------------------------------------------------------------|
|Negative|Allocate resources. Block the calling process until the value of the semaphore is greater than or equal to the absolute value of `sem_op`. (That is, wait until enough resources have been freed by other processes for this one to allocate.)  Then add (effectively subtract, since it's negative) the value of `sem_op` to the semaphore's value.|
|Positive|Release resources. The value of `sem_op` is added to the semaphore's value.|
|Zero|This process will wait until the semaphore in question reaches 0.|

So, basically, what you do is load up a `struct sembuf` with whatever
values you want, then call `semop()`, like this:

<!-- BOOKMARK -->

``` {.c}
int semop(int semid, struct sembuf *sops,
          unsigned int nsops);
```

The `semid` argument is the number obtained from the call to `semget()`.
Next is `sops`, which is a pointer to the `struct sembuf` that you
filled with your semaphore commands. If you want, though, you can make
an array of `struct sembuf`s in order to do a whole bunch of semaphore
operations at the same time. The way `semop()` knows that you're doing
this is the `nsop` argument, which tells how many `struct sembuf`s
you're sending it. If you only have one, well, put `1` as this argument.

One field in the `struct sembuf` that I haven't mentioned is the
`sem_flg` field which allows the program to specify flags to further
modify the effects of the `semop()` call.

One of these flags is `IPC_NOWAIT` which, as the name suggests, causes
the call to `semop()` to return with error `EAGAIN` if it encounters a
situation where it would normally block. This is good for situations
where you might want to "poll" to see if you can allocate a resource.

Another very useful flag is the `SEM_UNDO` flag. This causes `semop()`
to record, in a way, the change made to the semaphore. When the program
exits, the kernel will automatically undo all changes that were marked
with the `SEM_UNDO` flag. Of course, your program should do its best to
deallocate any resources it marks using the semaphore, but sometimes
this isn't possible when your program gets a `SIGKILL` or some other
awful crash happens.

<!-- ======================================================= -->
<!-- Destroying a semaphore -->
<!-- ======================================================= -->

## Destroying a semaphore

There are two ways to get rid of a semaphore: one is to use the Unix
command `ipcrm`. The other is through a call to `semctl()` with the
`cmd` set to `IPC_RMID`.

Basically, you want to call `semctl()` and set `semid` to the semaphore
ID you want to axe. The `cmd` should be set to `IPC_RMID`, which tells
`semctl()` to remove this semaphore set. The parameter `semnum` has no
meaning in the `IPC_RMID` context and can just be set to zero.

Here's an example call to torch a semaphore set:

``` {.c}
int semid; 
.
.
semid = semget(...);
.
.
semctl(semid, 0, IPC_RMID);
```

Easy peasy.

<!-- ======================================================= -->
<!-- Semaphore: Sample Programs -->
<!-- ======================================================= -->

## Sample Programs

There are two of them. The first, `semdemo.c`, creates the semaphore if
necessary, and performs some pretend file locking on it in a demo very
much like that in the [File Locking](#flocking) document. The second
program, `semrm.c` is used to destroy the semaphore (again, `ipcrm`
could be used to accomplish this.)

The idea is to run run `semdemo.c` in a few windows and see how all the
processes interact. When you're done, use `semrm.c` to remove the
semaphore. You could also try removing the semaphore while running
`semdemo.c` just to see what kinds of errors are generated.

Here's [flx[`semdemo.c`|semdemo.c]], including a function named
`initsem()` that gets around the semaphore race conditions,
Stevens-style:

``` {.c .numberLines}
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX_RETRIES 10

#ifdef NEED_SEMUN
/* Defined in sys/sem.h as required by POSIX now */
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
#endif

/*
** initsem() -- more-than-inspired by W. Richard Stevens' UNIX Network
** Programming 2nd edition, volume 2, lockvsem.c, page 295.
*/
int initsem(key_t key, int nsems)  /* key from ftok() */
{
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    if (semid >= 0) { /* we got it first */
            sb.sem_op = 1; sb.sem_flg = 0;
            arg.val = 1;

            printf("press return\n"); getchar();

            for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) { 
                    /* do a semop() to "free" the semaphores. */
                    /* this sets the sem_otime field, as needed below. */
                    if (semop(semid, &sb, 1) == -1) {
                            int e = errno;
                            semctl(semid, 0, IPC_RMID); /* clean up */
                            errno = e;
                            return -1; /* error, check errno */
                    }
            }

    } else if (errno == EEXIST) { /* someone else got it first */
            int ready = 0;

            semid = semget(key, nsems, 0); /* get the id */
            if (semid < 0) return semid; /* error, check errno */

            /* wait for other process to initialize the semaphore: */
            arg.buf = &buf;
            for(i = 0; i < MAX_RETRIES && !ready; i++) {
                    semctl(semid, nsems-1, IPC_STAT, arg);
                    if (arg.buf->sem_otime != 0) {
                            ready = 1;
                    } else {
                            sleep(1);
                    }
            }
            if (!ready) {
                    errno = ETIME;
                    return -1;
            }
    } else {
            return semid; /* error, check errno */
    }

    return semid;
}

int main(void)
{
    key_t key;
    int semid;
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = -1;  /* set to allocate resource */
    sb.sem_flg = SEM_UNDO;

    if ((key = ftok("semdemo.c", 'J')) == -1) {
            perror("ftok");
            exit(1);
    }

    /* grab the semaphore set created by seminit.c: */
    if ((semid = initsem(key, 1)) == -1) {
            perror("initsem");
            exit(1);
    }

    printf("Press return to lock: ");
    getchar();
    printf("Trying to lock...\n");

    if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            exit(1);
    }

    printf("Locked.\n");
    printf("Press return to unlock: ");
    getchar();

    sb.sem_op = 1; /* free resource */
    if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            exit(1);
    }

    printf("Unlocked\n");

    return 0;
}
```

Here's [flx[`semrm.c`|semrm.c]] for removing the semaphore when you're
done:

``` {.c .numberLines}
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main(void)
{
    key_t key;
    int semid;
    union semun arg;

    if ((key = ftok("semdemo.c", 'J')) == -1) {
            perror("ftok");
            exit(1);
    }

    /* grab the semaphore set created by seminit.c: */
    if ((semid = semget(key, 1, 0)) == -1) {
            perror("semget");
            exit(1);
    }

    /* remove it: */
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
            perror("semctl");
            exit(1);
    }

    return 0;
}
```

Isn't that fun! I'm sure you'll give up Quake^[Or whatever the current
addictive FPS game is these days.] just to play with this semaphore
stuff all day long!

<!-- ======================================================= -->
<!-- Semaphore summary -->
<!-- ======================================================= -->

## Summary

I might have understated the usefulness of semaphores. I assure you,
they're very very very useful in a concurrency situation. They're often
faster than regular file locks, too. Also, you can use them on other
things that aren't files, such as [Shared Memory Segments](#shm)! In
fact, it is sometimes hard to live without them, quite frankly.

Whenever you have multiple processes running through a critical
section of code, man, you need semaphores. You have zillions of
them---you might as well use 'em.

