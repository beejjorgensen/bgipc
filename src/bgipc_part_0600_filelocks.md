<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- File Locking -->
<!-- ======================================================= -->

# File Locking {#flocking}

File locking provides a very simple yet incredibly useful mechanism for
coordinating file accesses. Before I begin to lay out the details, let
me fill you in on some file locking secrets:

There are two types of locking mechanisms: mandatory and advisory.
Mandatory systems will actually prevent `read()`s and `write()`s to
file. Several Unix systems support them. Nevertheless, I'm going to
ignore them throughout this document, preferring instead to talk solely
about advisory locks. With an advisory lock system, processes can still
read and write from a file while it's locked. Useless? Not quite, since
there is a way for a process to check for the existence of a lock before
a read or write. See, it's a kind of _cooperative_ locking system. This
is easily sufficient for almost all cases where file locking is
necessary.

Since that's out of the way, whenever I refer to a lock from now on in
this document, I'm referring to advisory locks. So there.

Now, let me break down the concept of a lock a little bit more. There
are two types of (advisory!) locks: read locks and write locks (also
referred to as shared locks and exclusive locks, respectively.) The way
read locks work is that they don't interfere with other read locks. For
instance, multiple processes can have a file locked for reading at the
same. However, when a process has an write lock on a file, no other
process can activate either a read or write lock until it is
relinquished. One easy way to think of this is that there can be
multiple readers simultaneously, but there can only be one writer at a
time.

One last thing before beginning: there are many ways to lock files in
Unix systems. System V likes `lockf()`, which, personally, I think
sucks. Better systems support `flock()` which offers better control over
the lock, but still lacks in certain ways. For portability and for
completeness, I'll be talking about how to lock files using `fcntl()`.
I encourage you, though, to use one of the higher-level `flock()`-style
functions if it suits your needs, but I want to portably demonstrate the
full range of power you have at your fingertips. (If your System V Unix
doesn't support the POSIX-y `fcntl()`, you'll have to reconcile the
following information with your `lockf()` man page.)

## Setting a lock

The `fcntl()` function does just about everything on the planet, but
we'll just use it for file locking. Setting the lock consists of filling
out a `struct flock` (declared in `fcntl.h`) that describes the type of
lock needed, `open()`ing the file with the matching mode, and calling
`fcntl()` with the proper arguments, _comme ça_:

``` {.c}
struct flock fl = {
    .l_type   = F_WRLCK,  /* F_RDLCK, F_WRLCK, F_UNLCK      */
    .l_whence = SEEK_SET, /* SEEK_SET, SEEK_CUR, SEEK_END   */
    .l_start  = 0,        /* Offset from l_whence           */
    .l_len    = 0,        /* length, 0 = to EOF             */
    // .l_pid             /* PID holding lock; F_RDLCK only */
};
int fd;

fd = open("filename", O_WRONLY);

fcntl(fd, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
```

What just happened? Let's start with the `struct flock` since the fields
in it are used to _describe_ the locking action taking place.  Here are
some field definitions:

|Field|Description|
|:------:|-------------------------------------------------------------|
|`l_type`|This is where you signify the type of lock you want to set. It's either `F_RDLCK`, `F_WRLCK`, or `F_UNLCK` if you want to set a read lock, write lock, or clear the lock, respectively.|
|`l_whence`|This field determines where the `l_start` field starts from (it's like an offset for the offset). It can be either `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`, for beginning of file, current file position, or end of file.|
|`l_start`|This is the starting offset in bytes of the lock, relative to `l_whence`.|
|`l_len`|This is the length of the lock region in bytes (which starts from `l_start` which is relative to `l_whence`.|
|`l_pid`|The process ID of the process holding the lock. This is set by the kernel when using the F_RDLCK command.|

In our example, we told it make a lock of type `F_WRLCK` (a write lock),
starting relative to `SEEK_SET` (the beginning of the file), offset `0`,
length `0` (a zero value means "lock to end-of-file), with the PID set
to `getpid()`.

The next step is to `open()` the file, since `flock()` needs a file
descriptor of the file that's being locked. Note that when you open the
file, you need to open it in the same _mode_ as you have specified in
the lock, as shown in the table, below. If you open the file in the
wrong mode for a given lock type, `fcntl()` will return `-1` and `errno`
will be set to `EBADF`.

|`.l_type`|Mode|
|:-:|-|
|`F_RDLCK`|`O_RDONLY` or `O_RDWR`|
|`F_WRLCK`|`O_WRONLY` or `O_RDWR`|

Finally, the call to `fcntl()` actually sets, clears, or gets the lock.
See, the second argument (the `cmd`) to `fcntl()` tells it what to do
with the data passed to it in the `struct flock`. The following list
summarizes what each `fcntl()` `cmd` does:

|`cmd`|Description|
|:--------:|-------------------------------------------------------|
|`F_SETLKW`|This argument tells `fcntl()` to attempt to obtain the lock requested in the `struct flock` structure. If the lock cannot be obtained (since someone else has it locked already), `fcntl()` will wait (block) until the lock has cleared, then will set it itself. This is a very useful command. I use it all the time.|
|`F_SETLK`|This function is almost identical to `F_SETLKW`. The only difference is that this one will not wait if it cannot obtain a lock. It will return immediately with `-1`. This function can be used to clear a lock by setting the `l_type` field in the `struct flock` to `F_UNLCK`.|
|`F_GETLK`|If you want to only check to see if there is a lock, but don't want to set one, you can use this command. It looks through all the file locks until it finds one that conflicts with the lock you specified in the `struct flock`. It then copies the conflicting lock's information into the `struct` and returns it to you. If it can't find a conflicting lock, `fcntl()` returns the `struct` as you passed it, except it sets the `l_type` field to `F_UNLCK`.|


In our above example, we call `fcntl()` with `F_SETLKW` as the argument,
so it blocks until it can set the lock, then sets it and continues.

## Clearing a lock

Whew! After all the locking stuff up there, it's time for something
easy: unlocking! Actually, this is a piece of cake in comparison. I'll
just reuse that first example and add the code to unlock it at the end:

``` {.c}
struct flock fl = {
    .l_type   = F_WRLCK,  /* F_RDLCK, F_WRLCK, F_UNLCK      */
    .l_whence = SEEK_SET, /* SEEK_SET, SEEK_CUR, SEEK_END   */
    .l_start  = 0,        /* Offset from l_whence           */
    .l_len    = 0,        /* length, 0 = to EOF             */
    // .l_pid             /* PID holding lock; F_RDLCK only */
};
int fd;

fd = open("filename", O_WRONLY);  /* get the file descriptor */
fcntl(fd, F_SETLKW, &fl);  /* set the lock, waiting if necessary */
.
.
.
fl.l_type   = F_UNLCK;  /* tell it to unlock the region */
fcntl(fd, F_SETLK, &fl); /* set the region to unlocked */
```

Now, I left the old locking code in there for high contrast, but you can
tell that I just changed the `l_type` field to `F_UNLCK` (leaving the
others completely unchanged!) and called `fcntl()` with `F_SETLK` as the
command. Easy!

## A demo program

Here, I will include a demo program, `lockdemo.c`, that waits for the
user to hit return, then locks its own source, waits for another return,
then unlocks it. By running this program in two (or more) windows, you
can see how programs interact while waiting for locks.

Basically, usage is this: if you run `lockdemo` with no command line
arguments, it tries to grab a write lock (`F_WRLCK`) on its source
(`lockdemo.c`). If you start it with any command line arguments at all,
it tries to get a read lock (`F_RDLCK`) on it.

[flx[Here's the source|lockdemo.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
        struct flock fl = {
            .l_type = F_WRLCK,
            .l_whence = SEEK_SET,
            .l_start = 0,
            .l_len = 0,
        };
    int fd;

    if (argc > 1) 
        fl.l_type = F_RDLCK;

    if ((fd = open("lockdemo.c", O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }

    printf("Press <RETURN> to try to get lock: ");
    getchar();
    printf("Trying to get lock...");

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl");
        exit(1);
    }

    printf("got lock\n");
    printf("Press <RETURN> to release lock: ");
    getchar();

    fl.l_type = F_UNLCK;  /* set to unlock same region */

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl");
        exit(1);
    }

    printf("Unlocked.\n");

    close(fd);

    return 0;
}
```

Compile that puppy up and start messing with it in a couple windows.
Notice that when one `lockdemo` has a read lock, other instances of the
program can get their own read locks with no problem. It's only when a
write lock is obtained that other processes can't get a lock of any
kind.

Another thing to notice is that you can't get a write lock if there are
any read locks on the same region of the file. The process waiting to
get the write lock will wait until all the read locks are cleared. One
upshot of this is that you can keep piling on read locks (because a read
lock doesn't stop other processes from getting read locks) and any
processes waiting for a write lock will sit there and starve. There
isn't a rule anywhere that keeps you from adding more read locks if
there is a process waiting for a write lock. You must be careful.

Practically, though, you will probably mostly be using write locks to
guarantee exclusive access to a file for a short amount of time while
it's being updated; that is the most common use of locks as far as I've
seen. And I've seen them all...well, I've seen one...a small one...a
picture---well, I've heard about them.

## Summary

Locks rule. Sometimes, though, you might need more control over your
processes in a producer-consumer situation. For this reason, if no
other, you should see the document on System V [semaphores](#semaphores)
(or POSIX, for that matter; they aren't identical) if your system
supports such a beast. They provide a more extensive and at least
equally function equivalent to file locks.

