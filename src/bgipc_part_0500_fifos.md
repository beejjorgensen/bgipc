<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- FIFOs -->
<!-- ======================================================= -->

# FIFOs {#fifos}

A FIFO ("First In, First Out", pronounced "Fy-Foh") is sometimes known
as a _named pipe_. That is, it's like a [pipe](#pipes), except that it
has a name! In this case, the name is that of a file that multiple
processes can `open()` and read and write to.

This latter aspect of FIFOs is designed to let them get around one of
the shortcomings of normal pipes: you can't grab one end of a normal
pipe that was created by an unrelated process. See, if I run two
individual copies of a program, they can both call `pipe()` all they
want and still not be able to speak to one another. (This is because you
must `pipe()`, then `fork()` to get a child process that can communicate
to the parent via the pipe.)  With FIFOs, though, each unrelated process
can simply `open()` the pipe and transfer data through it.

## A New FIFO is Born

Since the FIFO is actually a file on disk, you have to do some
fancy-schmancy stuff to create it. It's not that hard. You just have to
call `mknod()` with the proper arguments. Here is a `mknod()` call that
creates a FIFO:

``` {.c}
mknod("myfifo", S_IFIFO | 0644 , 0);
```

In the above example, the FIFO file will be called "`myfifo`". The
second argument is the creation mode, which is used to tell `mknod()` to
make a FIFO (the `S_IFIFO` part of the OR) and sets access permissions
to that file (octal 644, or `rw-r--r--`) which can also be set by ORing
together macros from `sys/stat.h`. This permission is just like the one
you'd set using the `chmod` command. Finally, a device number is passed.
This is ignored when creating a FIFO, so you can put anything you want
in there.

(An aside: a FIFO can also be created from the command line using the
Unix `mknod` command.)

## Producers and Consumers

Once the FIFO has been created, a process can start up and open it for
reading or writing using the standard `open()` system call.

Since the process is easier to understand once you get some code in your
belly, I'll present here two programs which will send data through a
FIFO. One is `speak.c` which sends data through the FIFO, and the other
is called `tick.c`, as it sucks data out of the FIFO.

Here is [flx[`speak.c`|speak.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "american_maid"

int main(void)
{
    char s[300];
    int num, fd;

    mknod(FIFO_NAME, S_IFIFO | 0666, 0);

    printf("waiting for readers...\n");
    fd = open(FIFO_NAME, O_WRONLY);
    printf("got a reader--type some stuff\n");

    while (gets(s), !feof(stdin)) {
        if ((num = write(fd, s, strlen(s))) == -1)
            perror("write");
        else
            printf("speak: wrote %d bytes\n", num);
    }

    return 0;
}
```

What `speak` does is create the FIFO, then try to `open()` it. Now, what
will happen is that the `open()` call will _block_ until some other
process opens the other end of the pipe for reading. (There is a way
around this---see [`O_NDELAY`](#fifondelay), below.) That process is
[flx[`tick.c`|tick.c]], shown here:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "american_maid"

int main(void)
{
    char s[300];
    int num, fd;

    mknod(FIFO_NAME, S_IFIFO | 0666, 0);

    printf("waiting for writers...\n");
    fd = open(FIFO_NAME, O_RDONLY);
    printf("got a writer\n");

    do {
        if ((num = read(fd, s, 300)) == -1)
            perror("read");
        else {
            s[num] = '\0';
            printf("tick: read %d bytes: \"%s\"\n", num, s);
        }
    } while (num > 0);

    return 0;
}
```

Like `speak.c`, `tick` will block on the `open()` if there is no one
writing to the FIFO. As soon as someone opens the FIFO for writing,
`tick` will spring to life.

Try it! Start `speak` and it will block until you start `tick` in
another window. (Conversely, if you start `tick`, it will block until
you start `speak` in another window.)  Type away in the `speak` window
and `tick` will suck it all up.

Now, break out of `speak`. Notice what happens: the `read()` in `tick`
returns 0, signifying EOF. In this way, the reader can tell when all
writers have closed their connection to the FIFO. "What?" you ask "There
can be multiple writers to the same pipe?"  Sure! That can be very
useful, you know. Perhaps I'll show you later in the document how this
can be exploited.

But for now, lets finish this topic by seeing what happens when you
break out of `tick` while `speak` is running. "Broken Pipe"! What does
this mean? Well, what has happened is that when all readers for a FIFO
close and the writer is still open, the writer will receiver the signal
SIGPIPE the next time it tries to `write()`. The default signal handler
for this signal prints "Broken Pipe" and exits. Of course, you can
handle this more gracefully by catching SIGPIPE through the `signal()`
call.

Finally, what happens if you have multiple readers? Well, strange things
happen. Sometimes one of the readers get everything. Sometimes it
alternates between readers. Why do you want to have multiple readers,
anyway?

## `O_NDELAY`! I'm UNSTOPPABLE! {#fifondelay}

Earlier, I mentioned that you could get around the blocking `open()`
call if there was no corresponding reader or writer. The way to do this
is to call `open()` with the `O_NDELAY` flag set in the mode argument:

``` {.c}
fd = open(FIFO_NAME, O_WRONLY | O_NDELAY);
```

This will cause `open()` to return `-1` if there are no processes that
have the file open for reading.

Likewise, you can open the reader process using the `O_NDELAY` flag, but
this has a different effect: all attempts to `read()` from the pipe will
simply return `0` bytes read if there is no data in the pipe. (That is,
the `read()` will no longer block until there is some data in the pipe.)
Note that you can no longer tell if `read()` is returning `0` because
there is no data in the pipe, or because the writer has exited. This is
the price of power, but my suggestion is to try to stick with blocking
whenever possible.

## Concluding Notes

Having the name of the pipe right there on disk sure makes it easier,
doesn't it? Unrelated processes can communicate via pipes! (This is an
ability you will find yourself wishing for if you use normal pipes for
too long.)  Still, though, the functionality of pipes might not be quite
what you need for your applications. [Message queues](#mq) might be more
your speed, if your system supports them.
