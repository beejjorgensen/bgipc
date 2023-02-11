<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Pipes -->
<!-- ======================================================= -->

# Pipes {#pipes}

There is no form of IPC that is simpler than pipes. Implemented on every
flavor of Unix, `pipe()` and [`fork()`](#fork) make up the functionality
behind the "`|`" in "`ls | more`". They are marginally useful for cool
things, but are a good way to learn about basic methods of IPC.

Since they're so very very easy, I shant spent much time on them. We'll
just have some examples and stuff.

## "These pipes are clean!"

Wait! Not so fast. I might need to define a "file descriptor" at this
point. Let me put it this way: you know about "`FILE*`" from `stdio.h`,
right? You know how you have all those nice functions like `fopen()`,
`fclose()`, `fwrite()`, and so on? Well, those  are actually high level
functions that are implemented using _file descriptors_, which use
system calls such as `open()`, `creat()`, `close()`, and `write()`.
File descriptors are simply `ints` that are analogous to `FILE*`'s in
`stdio.h`.

For example, `stdin` is file descriptor "0", `stdout` is "1", and
`stderr` is "2". Likewise, any files you open using `fopen()` get their
own file descriptor, although this detail is hidden from you. (This file
descriptor can be retrived from the `FILE*` by using the `fileno()`
macro from `stdio.h`.)

![How a pipe is organized.](pipe1.pdf "[How a pipe is organized]")

Basically, a call to the `pipe()` function returns a pair of file
descriptors. One of these descriptors is connected to the write end of
the pipe, and the other is connected to the read end. Anything can be
written to the pipe, and read from the other end in the order it came
in. On many systems, pipes will fill up after you write about 10K to
them without reading anything out.

As a [flx[useless example|pipe1.c], the following program creates,
writes to, and reads from a pipe.

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];
    char buf[30];

    if (pipe(pfds) == -1) {
        perror("pipe");
        exit(1);
    }

    printf("writing to file descriptor #%d\n", pfds[1]);
    write(pfds[1], "test", 5);
    printf("reading from file descriptor #%d\n", pfds[0]);
    read(pfds[0], buf, 5);
    printf("read \"%s\"\n", buf);

    return 0;
}
```

As you can see, `pipe()` takes an array of two `int`s as an argument.
Assuming no errors, it connects two file descriptors and returns them in
the array. The first element of the array is the reading-end of the
pipe, the second is the writing end.

## `fork()` and `pipe()`---you have the power!

From the above example, it's pretty hard to see how these would even be
useful. Well, since this is an IPC document, let's put a `fork()` in the
mix and see what happens. Pretend that you are a top federal agent
assigned to get a child process to send the word "test" to the parent.
Not very glamorous, but no one ever said computer science would be the
X-Files, Mulder.

First, we'll have the parent make a pipe. Secondly, we'll `fork()`.
Now, the `fork()` man page tells us that the child will receive a copy
of all the parent's file descriptors, and this includes a copy of the
pipe's file descriptors. _Alors_, the child will be able to send stuff
to the write-end of the pipe, and the parent will get it off the
read-end. [flx[Like this|pipe2.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];
    char buf[30];

    pipe(pfds);

    if (!fork()) {
        printf(" CHILD: writing to the pipe\n");
        write(pfds[1], "test", 5);
        printf(" CHILD: exiting\n");
        exit(0);
    } else {
        printf("PARENT: reading from pipe\n");
        read(pfds[0], buf, 5);
        printf("PARENT: read \"%s\"\n", buf);
        wait(NULL);
    }

    return 0;
}
```

Please note, your programs should have a lot more error checking than
mine do. I leave it out on occasion to help keep things clear.

Anyway, this example is just like the previous one, except now we
`fork()` of a new process and have it write to the pipe, while the
parent reads from it. The resultant output will be something similar to
the following:

``` {.default}
PARENT: reading from pipe
 CHILD: writing to the pipe
 CHILD: exiting
PARENT: read "test"
```

In this case, the parent tried to read from the pipe before the child
writes to it. When this happens, the parent is said to _block_, or
sleep, until data arrives to be read. It seems that the parent tried to
read, went to sleep, the child wrote and exited, and the parent woke up
and read the data.

Hurrah!! You've just done some interprocess communication! That was
dreadfully simple, huh? I'll bet you are still thinking that there
aren't many uses for `pipe()` and, well, you're probably right. The
other forms of IPC are generally more useful and are often more exotic.

## The search for Pipe as we know it

In an effort to make you think that pipes are actually reasonable
beasts, I'll give you an example of using `pipe()` in a more familiar
situation. The challenge: implement "`ls | wc -l`" in C.

This requires usage of a couple more functions you may never have heard
of: `exec()` and `dup()`. The `exec()` family of functions replaces the
currently running process with whichever one is passed to `exec()`.
This is the function that we will use to run `ls` and `wc -l`. `dup()`
takes an open file descriptor and makes a clone (a duplicate) of it.
This is how we will connect the standard output of the `ls` to the
standard input of `wc`. See, stdout of `ls` flows into the pipe, and the
stdin of `wc` flows in from the pipe. The pipe fits right there in the
middle!

Anyway, [flx[here is the code|pipe3.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    int pfds[2];

    pipe(pfds);

    if (!fork()) {
        close(1);       /* close normal stdout */
        dup(pfds[1]);   /* make stdout same as pfds[1] */
        close(pfds[0]); /* we don't need this */
        execlp("ls", "ls", NULL);
    } else {
        close(0);       /* close normal stdin */
        dup(pfds[0]);   /* make stdin same as pfds[0] */
        close(pfds[1]); /* we don't need this */
        execlp("wc", "wc", "-l", NULL);
    }

    return 0;
}
```

I'm going to make another note about the `close()`/`dup()` combination
since it's pretty weird. `close(1)` frees up file descriptor 1 (standard
output). `dup(pfds[1])` makes a copy of the write-end of the pipe in the
first available file descriptor, which is "1", since we just closed
that. In this way, anything that `ls` writes to standard output (file
descriptor 1) will instead go to `pfds[1]` (the write end of the pipe).
The `wc` section of code works the same way, except in reverse.

## Summary

There aren't many of these for such a simple topic. In fact, there are
nearly just about none. Probably the best use for pipes is the one
you're most accustomed to: sending the standard output of one command to
the standard input of another. For other uses, it's pretty limiting and
there are often other IPC techniques that work better.

