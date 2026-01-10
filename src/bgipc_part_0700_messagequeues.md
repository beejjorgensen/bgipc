<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

# POSIX Message Queues {#mq}

Back in the day, we just had [System V message queues](#svmq), but the
friendly folks at [flw[POSIX|POSIX]] have standardized these things
somewhat so we can make more portable use of them.

And so here we are today in the glorious year %YEAR% and we'll soon see the
destructive power of this fully operational battle station. [*Darth
Vadar breathing sounds*].

Sorry. Got carried away, there. What are we doing?

## What is a Message Queue?

In general, we'd like to be able to send out *messages* (arbitrary
chunks of bytes) into *something*, and then have other processes receive
those messages.

And maybe we'd like those to be in some kind of order, like
*first-in-first-out* like those FIFO things we've already talked about.

Luckily, a queue is a FIFO data structure, and also luckily we have a
message we want to send. Message. Queue. Message queue!

So we'll have one (or many) *senders* pouring messages into the queue at
one end, and we'll have one (or many) *receivers* reading messages out
of the queue at the other end.

## Why This?

We have some benefits over a vanilla FIFO here. One is that messages
won't be split up (interleaved) if there are a lot of senders trying to
send at once. (Which could happen in a FIFO with larger messages.)

Another is that we can give these messages a _priority level_ to control
the order in which they are delivered.

Finally, like FIFOs, these queues can be joined or left at any time. New
processes merely have to open the queue by a well-known
previously-agreed-upon name. More on that soon.

## Priority

Every time you send a message to a queue, you attach a _priority_ that
indicates (vaguely) how quickly it should be delivered. The priority is
just an unsigned integer, where `0` is the lowest priority, and some
larger integer, indicated by `MQ_PRIO_MAX` is the highest.

The spec doesn't spell out the highest level other than that
system-dependent macro value. The Linux man page suggests keeping
priority levels between 0 and 31, inclusive, to remain as portable as
possible.

And if you do need more than 32 priority levels... honestly, what are
you building?

*Anyway*, when you receive a message, you'll get the one that was sent
with the highest priority (even if it was sent later that others with
lower priorities). If there's a tie for highest priority, the messages
battle to the death.

No, that's not right. If there's a tie in priority, the tied messages
are de-queued in the order they were sent (FIFO).

## Identifying a Queue

Message queues are identified by a _name_, which is a string that should
begin with a slash (`/`) and not have any other slashes in it. (Things
get all "implementation defined" if you violate those rules.)

So for example, here's a queue name: `/waco_kid`. Very exciting. All the
programs who wanted to use that queue would have to know that name in
advance so they could open it.

## General Approach

### Open the Queue

Both the sender and receiver have to the do the same thing up front:
open (connect to) the message queue. This is done with the `mq_open()`
system call. (And here you'll see the `mqueue.h` header file you'll need
for all these.)

The queue is also created with this call. If it doesn't yet exist and
the proper flags and arguments are sent to `mq_open()`, the queue will
be created then.

It's notable at this time that the queue you create never goes away
until you throw away your computer or you _unlink_ the queue, whichever
comes first. More on unlinking later on.

Let's see this syscall!

``` {.c}
#include <mqueue.h>
#include <fcntl.h>  // For the O_ flags

mqd_t mq_open(const char *name, int oflag, ...);
```

So you give it the name as the first argument, e.g. `/waco_kid` and then
you pass some flags in `oflag`. And depending on the flags, maybe you
pass some more stuff with those scary ellipses these at the end.

The flags are bitwise-ORd together. First, you have to say if you want
to open it for reading (receiving), writing (sending), or both. Also,
you can tell it if you want to create the queue if it doesn't exist. And
you can tell it if the queue should be _blocking_ or not. More on that
later.

|Flag|Description|
|-|-|
|`O_RDONLY`|Open for receiving only|
|`O_WRONLY`|Open for sending only|
|`O_RDWR`|Open for both receiving and sending|
|`O_CREAT`|Create the queue if it doesn't exist|
|`O_NONBLOCK`|Create a non-blocking queue|

For example:

``` {.c}
mqd_t mq = mq_open("/waco_kid", O_RDONLY);
```

But here we get to those ellipses! If we specify `O_CREAT`, we get to do
more things!

In particular, we get to set the permissions (who is allowed to connect
to this), which we do just like any other standard Unix file
permissions. In the example, below, we use `0644` permissions, which is
`rw-r--r--`. And then I put a `NULL` for the fourth argument—we'll soon
see what that means.

``` {.c}
mqd_t mqdes = mq_open("/waco_kid", O_RDONLY | O_CREAT, 0644, NULL);
```

As is, that'll create a message queue! And it does it with a default
maximum number of messages and maximum message size.

What if you want something other than the default? You can use that
fourth argument to specify with a `struct mq_attr`.

Here are the pertinent fields for queue creation:

``` {.c}
struct mq_attr {
    long mq_maxmsg;    // Max message count
    long mq_msgsize;   // Max message size
}
```

You can control how many messages can be in the queue at a time with
`mq_maxmsg`. There's no definite maximum for this in the spec, but the
most you can specify on my Linux machine is 10. That seems kind of low,
but you have to imagine that the kernel is just holding onto all these
messages until someone receives them, and it doesn't want to just use
all your memory doing so. If things are working smoothly, other
processes should be consuming the messages as quickly as you're
producing them.

> And, as we all know, *everything **always** runs smoothly!*

In addition, each message can't be larger than the `mq_msqsize`. Again,
no defined maximum, but on my Linux machine it's 8 KB.

> **You can find these out for yourself** on Linux by looking at some
> files in `/proc`
>
> ``` {.default}
> cat /proc/sys/fs/mqueue/msgsize_max   # max mq_msgsize
> cat /proc/sys/fs/mqueue/msg_max       # max mq_maxmsg
> cat /proc/sys/fs/mqueue/queues_max    # max queues
> ```

<!-- ` -->

We'll see what else we can do with a `struct mq_attr` later, including
inspecting how many messages there are in the queue.

### Send Things to the Queue

OK! Now that we have the queue open and created, we can send stuff to
it!

``` {.c}
#include <mqueue.h>

int mq_send(mqd_t mqdes, const char *msg_ptr,
            size_t msg_len, unsigned int msg_prio);
```

That sends the message pointed to by `msg_ptr` (which is `msg_len` bytes
long) to the queue identifier we got back from `mq_open()`. And it sends
it with priority `msg_prio`.

That's it. Here's an example with no error checking:

``` {.c}
mqd_t mqdes = mq_open("/waco_kid", O_RDONLY | O_CREAT, 0644, NULL);

mq_send(mqdes, "Play chess", 10, 0);
```

If you send when the queue is full, it will block until something
removes a message from the queue to make room.

### Receive Things from the Queue

The flip-side is receiving things. Pretty much the same deal.

``` {.c}
#include <mqueue.h>

ssize_t mq_receive(mqd_t mqdes, char msg_ptr[msg_len],
                   size_t msg_len, unsigned int *msg_prio);
```

That will receive a message from the queue identified by `mqdes`. It
stores the message in `msg_ptr`, which better be a buffer of at least
`msg_len` bytes in size, or else. Oh, and `msg_len` better be at least
as big as the maximum message size (that you optionally set with
`mq_open()`), or else, again.

Finally, if you're interested in the priority of this message, you can
pass a pointer to an `unsigned int` in `msg_prio` to hold it. Or you can
pass `NULL` for that argument if you don't care.

``` {.c}
mqd_t mqdes = mq_open("/waco_kid", O_RDONLY);

char msg[8192];
ssize_t recv_len;
unsigned int msg_prio;

recv_len = mq_receive(mqdes, msg, sizeof msg, &msg_prio);

// Print it to stdout
write(1, msg, recv_len);
```

Again, you should error-check those.

### Close the Queue

When you're done with the queue *in one particular process*, you can
close it. (The queue will continue to exist until unlinked.)

``` {.c}
#include <mqueue.h>

int mq_close(mqd_t mqdes);
```

Pretty straightforward. Here's an example for completeness:

``` {.c}
mqd_t mqdes = mq_open("/waco_kid", O_RDONLY);

// ...
// Do queue things for a while until we're done.
// ...

mq_close(mqdes);
```

## Example Sender

Let's get a complete example. This code will prompt you for a message
priority and message separated by a space, like `5 Hello`. Enter a blank
line to quit.

And it'll send the null-terminated string out to the queue.

``` {.c}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>

/**
 * Input a priority and message from the keyboard.
 *
 * Really fragile--for demo purposes only.
 */
int input(char *buf, size_t bufsize, unsigned int *msg_prio)
{
    printf("Priority and message (e.g. 2 hi): ");
    fflush(stdout);
    fgets(buf, bufsize - 1, stdin);
    buf[bufsize - 1] = '\0';

    char *token = strtok(buf, " \n");

    if (token == NULL)
        return 0;
    
    *msg_prio = atoi(token);  // Get priority

    token = strtok(NULL, "\n");
    int msg_len = strlen(token) + 1;

    memmove(buf, token, msg_len);

    return msg_len;
}

int main(void)
{
    char msg[128];

    struct mq_attr attr = {
        .mq_maxmsg = 3,
        .mq_msgsize = 256
    };

    mqd_t mqdes = mq_open("/mq_test", O_WRONLY | O_CREAT, 0644,
                          &attr);

    for (;;) {
        unsigned int msg_prio;

        int msg_len = input(msg, sizeof msg, &msg_prio);

        if (msg_len == 0)
            break;

        printf("sending \"%s\" (%d bytes) at priority %u\n", msg,
               msg_len, msg_prio);

        if (mq_send(mqdes, msg, msg_len, msg_prio) == -1) {
            perror("mq_send");
        }
    }

    mq_close(mqdes);
}
```

Run that and send some stuff. Note that the maximum number of messages
in the queue at a time is set to `3`, so when you try to send the fourth
thing, it'll block until you fire up a receiver.

``` {.default}
$ ./mq_sender
  Priority and message (e.g. 2 hi): 1 hello
  sending "hello" (6 bytes) at priority 1
  Priority and message (e.g. 2 hi): 2 and this
  sending "and this" (9 bytes) at priority 2
  Priority and message (e.g. 2 hi): 0 low priority
  sending "low priority" (13 bytes) at priority 0
  Priority and message (e.g. 2 hi): 2 a fourth message
  sending "a fourth message" (17 bytes) at priority 2
```

And there I've blocked. Let it just sit there for now, and let's fire up
a receiver in another terminal.

## Example Receiver

Here's a receiver of messages.

``` {.c}
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    mqd_t mqdes = mq_open("/mq_test", O_RDONLY);

    char msg[128];
    char msg_len = sizeof msg;
    unsigned int msg_prio;
    ssize_t bytes_received;

    for (;;) {
        bytes_received = mq_receive(mqdes, msg, msg_len,
                                    &msg_prio);

        if (bytes_received == -1) {
            perror("mq_receive");
            return 1;
        }

        printf("received \"%s\" (%zd bytes) at priority %u\n", msg,
               bytes_received, msg_prio);
    }

    mq_close(mqdes);
}
```

Assuming that you still have the sender from above running in one
window, and you just launched this one in another window, you'll
immediately see two things.

One is that the receiver will gobble up and print all the messages in
the queue. The other is the sender will immediately unblock and give you
a chance to type another message in.

The receiver should print this:

``` {.default}
$ ./mq_receiver
  received "and this" (9 bytes) at priority 2
  received "a fourth message" (17 bytes) at priority 2
  received "hello" (6 bytes) at priority 1
  received "low priority" (13 bytes) at priority 0
```

Notice the order! The fourth message we sent actually arrived second.
Why? Because it's priority 2, so it gets received before anything of
lower priority, even if those lower-priority messages were sent earlier.
Line-cutter!

And at this point, if you type things in the sender, they should
immediately arrive on the receiver.

## Multiple Processes

If you have multiple senders, they'll all try to dump messages into the
same queue as makes visceral sense. No biggie.

If you have multiple receivers, I'm not actually sure what the
specification says about it. But when I run it on Linux, it seems like
receivers alternate receiving messages.

And this is sensible behavior. Maybe you have one process creating jobs
and putting them in the queue, and you have multiple processes running
jobs, all of which reach into the queue for the next thing to do.

Try it! Open yet another window, fire up a second receiver, and see
where the messages from the sender go.

## Unlinking (Deleting) the Queue

If all the programs `mq_close()` the queue, does it go away? ***No***,
it does not. It stays around. And if there are things in it, they stick
around, too.

You have to _unlink_ the queue, which is somewhat analogous to deleting
a file.

``` {.c}
#include <mqueue.h>

int mq_unlink(const char *name);
```

You just give it the same name that you created it with:

``` {.c}
mq_unlink("/waco_kid");
```

And that's it.

Kind of. There are some devilish details. If you unlink the queue, it
actually continues to exist until all users of the queue have exited
or `mq_close()`d it.

So if *everyone* has closed it **and** you unlink it, then it's gone.

Also if you unlink it but keep it open, another process can create a
different queue of the same name that you used.

> This is actually exactly how file deletion (which uses the `unlink()`
> syscall) works. You can unlink a file and keep it open; the file
> doesn't actually get removed from the disk until it has been unlinked
> **and** all processes have closed it.

Here's an example that unlinks the message queue from the previous
examples. If you don't run this, the queue will persist until you
reboot.

``` {.c}
#include <stdio.h>
#include <mqueue.h>

int main(void)
{
    if (mq_unlink("/mq_test") == -1) {
        perror("/mq_test");
        return 1;
    }
}
```

## Queue Metadata

You can look at the attributes for the queue, a couple of which you
might have set in your `mq_open()` call.

These calls use our old friend `struct mq_attr` to hold the information.

``` {.c}
struct mq_attr {
    long mq_flags;     // O_NONBLOCK?
    long mq_maxmsg;    // Max message count
    long mq_msgsize;   // Max message size
    long mq_curmsgs;   // How many messages in queue
};
```

You can see if the queue was created as non-blocking by looking at
`mq_flags`. If you bitwise-AND that with `O_NONBLOCK` and get non-zero,
it's non-blocking. Non, non, non.

And, obviously, you can see how full the queue is by looking at
`mq_curmsgs`.

You can also set the attributes, but the only thing you're allowed to
set is `mq_flags`. So this is the way you can change a queue from
blocking to non-blocking or vice-versa. All other fields are ignored
when setting the attributes.

Here's the getter and setter:

``` {.c}
#include <mqueue.h>

int mq_getattr(mqd_t mqdes, struct mq_attr *attr);

int mq_setattr(mqd_t mqdes, const struct mq_attr *newattr,
               struct mq_attr *oldattr);
```

The setter also gives you the previous attributes back in `oldattr`, if
it's not `NULL`.

Here's an example that looks at how many messages in the queue and then
changes it to non-blocking.

``` {.c}
struct mq_attr attr;

mq_getattr(mqdes, &attr);

printf("Currently in queue: %ld\n", attr.mq_curmsgs);

attr.mq_flags |= O_NONBLOCK;
mq_setattr(mqdes, &attr, NULL);
```

## Time out!

I'm not going to get into too much detail here, but with these calls
that block, we might want to actually only wait a certain amount of time
before giving up.

You can use the `mq_timedsend()` and `my_timedreceive()` calls that work
just like `mq_send()` and `mq_receive()` except they have a `struct
timespec` at the end that allows you to specify a timeout.

If there is a timeout, the call returns `-1` and `errno` is set to
`ETIMEDOUT`.

For a quick refresher, `struct timespec` has two fields:

* `tv_sec` number of seconds, plus...
* `tv_nsec` number of nanoseconds

There are 1,000,000,000 (a billion) nanoseconds in a second, so the
`tv_nsec` field goes from `0` to `999999999`.

Here's a `struct timespec` that gives you a 3.75-second timeout:

``` {.c}
struct timespec timelimit = {
    .tv_sec = 3,
    .tv_nsec = 750000000
}
```

## Blocking and Non-Blocking

In general, if you try to send a message to a queue that's full, the
`mq_send()` call will block.

And if you try to receive from a queue that is empty with
`mq_receive()`, the call will block.

If that's not desirable, you can set the queue to non-blocking. You do
this either by passing the `O_NONBLOCK` flag to `mq_open()`, or by
setting it after the fact with `mq_setattr()`.

If you set the queue as non-blocking, all those calls that *would* block
normally will return `-1` and `errno` will be set to `EWOULDBLOCK`.

