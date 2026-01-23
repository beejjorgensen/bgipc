<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Signals Part 2 -->
<!-- ======================================================= -->

# Signals Part II

In this section of the guide, we're going to look at how to block
signals, and some best practices for writing signal handler functions
without getting into serious trouble. But first, let's have some
devilish details.

## Edge Cases

Let's talk weird stuff.

What happens if your signal handler is running and another signal
arrives? Sensibly, by default, the second signal is deferred until after
the signal handler finishes[^a399].

[^a399]: You can override this with `SA_NODEFER` in your `sa_flags`, but
    that's a sure path to madness.

OK, then... What happens if there's already a deferred signal and
another one arrives? In that case, the two signals are collapsed into a
single one and only one will arrive! If you get a signal, you can be
sure that it was raised one or more times before your handler saw it.

So don't expect a *count*. When your handler is called, all you can be
sure of is that the signal was raised at least once.

Now back to the fun stuff.

## Blocking Signals

You can block signals from arriving. This doesn't discard the signal; it
just holds it off for a while. If you're blocking a signal and it
arrives, nothing will happen... until you unblock it and then it will
arrive immediately.

You do this with the `sigprocmask()` call[^e0e0]. This manipulates the
per-process table of signals that are blocked.

[^e0e0]: If you're using POSIX threads, use the equivalent
    `pthread_sigmask()`, instead, to do this on a per-thread basis.

Here's the prototype:

``` {.c}
#include <signal.h>

int sigprocmask(int how, const sigset_t *restrict set,
                sigset_t *restrict oset);
```

That's a bit messy, but `how` is saying "block or unblock". And `set` is
the set of signals to block. Finally, `oset` is the *previous* set of
blocked signals so you can switch back to it later. You can make `oset`
`NULL` if you don't care about the previous set.

The `how` field can be set to three amazing things:

|`how`|Description|
|-----|----------------------------------------------------|
|`SIG_BLOCK`|Add signals to the current list of blocked signals.|
|`SIG_UNBLOCK`|Remove signals from the current list of blocked signals.|
|`SIG_SETMASK`|Set the current list of blocked signals to exactly this.|

So let's give it a shot in this demo, [flx[`sigblock.c`|sigblock.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(void)
{
    sigset_t mask, oldmask;

    // Make a set with SIGINT in it
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    // Block everything in that set
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    // SIGINT is blocked for now!
    puts("Try to ^C out of here! You can't for 5 seconds!");
    sleep(5);

    // Back to how it was before, presumably without blocking SIGINT
    puts("Ok, now you can.");
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    puts("If you hit ^C, this won't print.");
}
```

If you hit `CTRL-C` during the `sleep()`, you'll find the program isn't
interrupted. You're generating `SIGINT`s, but they're blocked. And
they'll be delivered as soon as they're unblocked, which happens with
the `sigprocmask(SIG_SETMASK...` on line 22.

And because they're unblocked and we're using the default handler (which
exits), the process will exit right after we unblock them, **before**
the last `puts()`.

## Signal Handler Function Practices

Since signal handler functions are so limited, the general pattern
programmers like is for the signal handler to really do nothing other
than notify the main code that has occurred, and that's all.

Let's look at a variant of an earlier example. Note this is **not** how
you should code this.

``` {.c}
volatile sig_atomic_t signal_happened;

void handler(int sig)
{
    signal_happened = 1;
}

int main(void)
{
    // ... signal setup ...

    while (!signal_happened) { /* spin */ }

    puts("Signal happened!");
    signal_happened = 0;

    // ... etc. ...
}
```

You don't want to do this because it just spins chewing up CPU like it's
going out of style while waiting for the signal. But it is a bare-bones
example of the general pattern. We just need to get rid of the
spin-wait.

This means that we'll put the main process to sleep somehow, for
example:

``` {.c}
while (!signal_happened) { sleep(1000000); }
```

That's better! Assuming you haven't specified `SA_RESTART`, the
`sleep()` will fail with `EINTR` the moment the signal is raised and
you'll break out of the loop. Sure, it wakes up to check every
eleven-and-a-half days, and that uses some CPU, but that's something I
can live with.

And, like we saw before, the great thing here is that the signal handler
didn't do anything except do an atomic write to a global. Everything
else is handled cleanly by the program so we don't have to worry about
non-atomic writes or race conditions.

But that program is so *dull*! It doesn't do anything!

What if we want our application to do things **and** handle signals?
Whoa, don't get crazy.

Well, guess what! We have options. I'm going to give two here, and you
can really use whatever fits. Both of them assume you're using something
like `select()` or `poll()` to handle asynchronous I/O events and that's
what's driving your program. Or, at least, they assume that you can
retrofit your code to do that.

And if you need a refresher, see [fl[_Beej's Guide to Network
Programming_|https://beej.us/guide/bgnet/]], in particular the sections
on
[fl[`poll()`|https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html#poll]]
and
[fl[`select()`|https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html#select]].

### Using a Pipe

If you are already using `select()` or `poll()` to wait for events, this
approach can work for you quite handily.

The idea is that you're going to make a pipe. The main process adds the
pipe's read end to its `select()` or `poll()` set of file descriptors
it's waiting on.

Then, when a signal arrives, the signal handler writes a simple
identifier into the pipe. Then the main process will return from
`select()` or `poll()` and you can see what's in the pipe. The
identifier lets you know what signal was handled.

Here's a snippet from the demo program [flx[`pipesig.c`|pipesig.c]]. It
waits for text entry from `stdin` as well as waiting for information to
arrive on the pipe. (In this case, we'll use `poll()`, but `select()`
would work just as well.) It launches a background process that raises
`SIGUSR1` on the parent process every few seconds.

``` {.c}
int pipefd[2];

void handler(int sig)
{
    (void)sig;
    write(pipefd[1], "1", 1);
}
```

That's it for the signal handler! It just puts an ASCII `1` into the
pipe. The end.

Let's look at how it's handled (code has been simplified here in the
text—view the full source to see how it works):

``` {.c}
struct pollfd pollfds[2] = {
    { .fd=0, .events=POLLIN },
    { .fd=pipefd[0], .events=POLLIN },
};

st = poll(pollfds, 2, 0);

// ...

if ((pollfds[0].revents & POLLIN)) {
    if (fgets(line, sizeof line, stdin) == NULL) return;

    int len = strlen(line);
    if (line[len-1] == '\n') line[len-1] = '\0';

    if (strcmp(line, "quit") == 0) return;

    printf("You entered: \"%s\"\n", line);
}

else if ((pollfds[1].revents & POLLIN)) {
    char sigdata[1024];

    int count = read(pipefd[0], sigdata, sizeof sigdata);

    for (int i = 0; i < count; i++)
        if (sigdata[i] == '1')
            printf("SIGUSR1 occurred\n");
}
```

There we set up our `pollfds` array to watch file descriptor `0`
(standard input likely from the keyboard) and file descriptor
`pipefd[0]`, the read end of the pipe.

If we get something from `stdin`, we handle that by printing it out. If
we get something on the pipe, we check the identifier and print out what
happened. (Clearly I have some issues here if more than 1024 signals
happen before I wake up to handle the `poll()`, but fixing that is left
as an exercise for you and your high-performance computing environment.)

Here's some output from a sample run:

``` {.default}
Enter lines of text, or "quit" to quit.
hi
You entered: "hi"
SIGUSR1 occurred
This is a long lSIGUSR1 occurred
ine of text
You entered: "This is a long line of text"
SIGUSR1 occurred
quit
Quitting, sending SIGTERM to child
```

It's pretty straightforward. Yes, the signal handler is calling
`write()` and using a global pipe descriptor that isn't atomic, but we
only set the pipe descriptor at the beginning of the run before the
signal handler is installed. And we don't modify it after that. So
relative safety is assured.

### Using `pselect()`

If you're already using `select()`, this might be an even cleaner way
than pipes to notify the process that a signal has occurred.

Some clever Unix hacker found themselves thinking this way: what if
there were a version of `select()` that could wake up when one of a
particular set of signals was raised? And it could do this in addition
to all the file descriptor monitoring it normally does?

And so they made that.

``` {.c}
#include <sys/select.h>

int pselect(int nfds,
            fd_set *restrict readfds,
            fd_set *restrict writefds,
            fd_set *restrict errorfds,
            const struct timespec *restrict timeout,
            const sigset_t *restrict sigmask);
```

Looks like `select()`, right? The only differences are:

* Timeout is a `struct timespec` instead of a `struct timeval`.
* We have that `sigmask` as a final parameter.

For the demo, we'll leave `timeout` as `NULL` so it never times out, but
you could absolutely add it if you wanted.

And the `sigmask` should hold a set of signals that are to be blocked
during the `pselect()` call... which should **not** include the signal
you're handing!

That seems like nonsense. Let's look at the overall approach the main
process will take:

1. Set up the signal handler, say for `SIGUSR1`.
2. Block `SIGUSR1` with `sigprocmask()`.
3. Call `pselect()` with a `sigmask` that does **not** include
   `SIGUSR1`.
4. When `pselect()` returns, check to see if it was due to a signal.

So if `SIGUSR1` is blocked, how does it get through? This the magic
beans part.

`pselect()` takes the `sigmask` you pass it and sets the process signal
mask to it. Let's say you pass it an empty set. In that case, no signals
would be blocked, and they'd all get through. So while you're in the
call to `pselect()`, `SIGUSR1` isn't blocked and it can work.

And then (for the other magic beans part), after the signal comes in,
`pselect()` *restores* the process signal mask to whatever it was
before.

The practical upshot of all this is that your process has blocked
`SIGUSR1` everywhere *except* while `pselect()` is being called! This
gives you central control over when to handle signals and what to do.

The pseudocode for `pselect()` looks vaguely like this:

``` {.python}
pselect(readset, timeout, sigmask):
    sigprocmask(SIG_SETMASK, sigmask, oldmask);
    select(readset, timeout)
    sigprocmask(SIG_SETMASK, oldmask, NULL);
```

The key feature is that, this being a syscall, this all happens
atomically from our perspective. We couldn't write this in user space
without is being all racy.

Let's look at the example [flx[`pselect.c`|pselect.c]], which is just
the same as the `poll()` example, above, except that it uses
`pselect()`. Here's the handler.

``` {.c}
volatile sig_atomic_t sigusr1_happened;

void handler(int sig)
{
    sigusr1_happened = 1;
}
```

Again, short and sweet. We just set a global atomic flag indicating the
signal happened. Let's look in the main chunk of code (again, edited for
brevity):

``` {.c}
sigset_t mask, oldmask;
sigemptyset(&mask);
sigaddset(&mask, SIGUSR1);
sigprocmask(SIG_BLOCK, &mask, &oldmask);

// ...

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
```

A few things to unpack, here.

* We make a new `mask` with `SIGUSR1` in it and we block that signal.
* We keep the old mask (which in this case is an empty set), and we'll
  use that as the set to block with `pselect()`.
* We add file descriptor `0` (`stdin`) to our `readfds` so that
  `pselect()` will return if we type something.
* We call `pselect()`.
* If it returns `-1` and `errno` is `EINTR`, it means `pselect()` was
  interrupted by a signal! We then test our global to see if it was our
  signal.
* If it returns positive (`0` would mean timeout), it must be one of our
  file descriptors. We test if it's file descriptor `0` (`stdin`), and,
  if it is, we read data with `fgets()`.

So, again, we have the signal handling stuff out in the main loop where
it's under our control and we don't have to deal with nasty concurrency
issues.

That `oldmask` stuff is pretty weird. By doing it this way, we're
basically telling `pselect()` that we only want to be notified with
`SIGUSR1` arrives and not some other signal. We block everything that
was already blocked before we added `SIGUSR1` to the set. (Which, in
this case, was no other signal, so `oldmask` is an empty set.)

## Conclusion

So there you have it. Some of the zany techniques we have for actually
dealing properly with POSIX signals. Big takeaways are that you can
handle all kinds of signals and you can block their delivery. Also you
should only modify globals in the signal handler if they're atomic. And
if the globals are written to anywhere at all while the handler is
armed, they should also be atomic.

And that if you want to handle signals properly, it really should be
taken care of in the main loop unless you're just ignoring them. And you
can use pipes or `pselect()` to help with this.

Code safe, and watch for dragons!
