<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Signals -->
<!-- ======================================================= -->

# Signals

There is a sometimes useful method for one process to bug another:
signals. Basically, one process can "raise" a signal and have it
delivered to another process. The destination process's signal handler
(just a function) is invoked and the process can handle it.

This is an interestingly-different mechanism than you might be used to
in that your program might be happily humming along doing whatever it
wants, and then a signal is raised and your program is interrupted. Your
code might be mid-function calculating π to 1.21 giga-decimal places and
suddenly it stops doing that, and control passes to another function
you've written (the _signal handler_) to handle the signal.

And when the signal handler returns, control jumps back to your π
calculation and continues where it left off. Or maybe the program just
terminates! It depends on the signal and if-and-how you've decided to
handle it.

The devil's in the details, of course, and in actuality what you are
permitted to do safely inside your signal handler is rather limited.
Nevertheless, signals provide a useful service.

For example, one process might want to temporarily stop another one, and
this can be done by sending the signal `SIGSTOP` to that process. To
continue, the process has to receive signal `SIGCONT`[^847f]. How does
the process know to do this when it receives a certain signal? Well,
many signals are predefined and the process has a default signal handler
to deal with it.

[^847f]: Fun fact: when you hit `CTRL-Z` in the terminal while you're
    running a program in the foreground, it sends a `SIGSTOP` to that
    process and the shell reports that it is stopped or suspended. If
    you then type `fg`, it'll bring that process back to the foreground
    and send it `SIGCONT` to keep running where it left off.

A default handler? Yes. Take `SIGINT` for example. This is the interrupt
signal that a process receives when the user hits `CTRL-C`. The default
signal handler for `SIGINT` causes the process to exit! Sound familiar?
Well, as you can imagine, you can override the `SIGINT` signal to do
whatever you want (or nothing at all!)  You could have your process
print "Interrupt?! No way, Jose!" and go about its merry business.

So now you know that you can have your process respond to just about any
signal in just about any way you want. Naturally, there are exceptions
because otherwise it would be too easy to understand. Take the ever
popular `SIGKILL`, signal #9. Have you ever typed "`kill -9 nnnn`" to
kill a runaway process number `nnnn`? You were sending it `SIGKILL`. Now
you might also remember that no process can get out of a "`kill -9`",
and you would be correct. `SIGKILL` is one of the signals you **can't**
add your own signal handler for. The aforementioned `SIGSTOP` is also in
this category.

(Aside: you often use the Unix `kill` command without specifying a
signal to send...so what signal is it? The answer: `SIGTERM`. You can
write your own handler for `SIGTERM` so your process won't respond to a
regular "`kill`", and the user must then use "`kill -9`" to end the
process.)

Are all the signals predefined? What if you want to send a signal that
has significance that only you understand to a process? There are two
signals that aren't reserved: `SIGUSR1` and `SIGUSR2`. You are free to
use these for whatever you want and handle them in whatever way you
choose. (For example, my CD player program might respond to `SIGUSR1` by
advancing to the next track. In this way, I could control it from the
command line by typing "`kill -SIGUSR1 nnnn`".)

## Catching Signals for Fun and Profit!

As you can guess the Unix "kill" command is one way to send signals to a
process. By sheer unbelievable coincidence, there is a system call
called `kill()` which does the same thing. It takes for its argument a
signal number (as defined in `signal.h`) and a process ID. Also, there
is a library routine called `raise()` which can be used to raise a
signal within the same process.

The burning question remains: how do you catch a speeding `SIGTERM`?
You need to call `sigaction()` and tell it all the gritty details about
which signal you want to catch and which function you want to call to
handle it.

Here's the `sigaction()` breakdown:

``` {.c}
int sigaction(int sig, const struct sigaction *act,
              struct sigaction *oact);
```

The first parameter, `sig` is which signal to catch. This can be
(probably "should" be) a symbolic name from `signal.h` along the lines
of `SIGINT`. That's the easy bit.

The next field, `act` is a pointer to a `struct sigaction` which has a
bunch of fields that you can fill in to control the behavior of the
signal handler. (A pointer to the signal handler function itself
included in the `struct`.)

Lastly `oact` can be `NULL`, but if not, it returns the _old_ signal
handler information that was in place before. This is useful if you want
to restore the previous signal handler at a later time.

We'll focus on these three fields in the `struct sigaction`:

|Signal|Description|
|------------|----------------------------------------------------------|
|`sa_handler`|The signal handler function|
|`sa_mask`|A set of signals to block while this one is being handled|
|`sa_flags`|Flags to modify the behavior of the handler, or `0`|

`sa_handler` is a pointer to a function that returns `void` and takes a
single `int` parameter (which will hold the signal number that it's
handling. You can also specify `SIG_IGN` to ignore the signal, or
`SIG_DEF` to set it to the default action.

What about that `sa_mask` field? When you're handling a signal, you
might want to block other signals from being delivered, and you can do
this by adding them to the `sa_mask` It's a "set", which means you can
do normal set operations to manipulate them: `sigemptyset()`,
`sigfillset()`, `sigaddset()`, `sigdelset()`, and `sigismember()`.  In
this example, we'll just clear the set and not block any other signals.

Examples always help! Here's one that handled `SIGINT`, which can be
delivered by hitting `^C`, called [flx[`sigint.c`|sigint.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void sigint_handler(int sig)
{
    (void)sig;
    const char msg[] = "Ahhh! SIGINT!\n";
    write(1, msg, sizeof msg  - 1);
}

int main(void)
{
    char s[200];
    struct sigaction sa = {
        .sa_handler = sigint_handler,
        .sa_flags = 0, // or SA_RESTART
    };
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Enter a string:\n");

    if (fgets(s, sizeof s, stdin) == NULL)
        perror("fgets");
    else 
        printf("You entered: %s\n", s);

    return 0;
}
```

This program has two functions: `main()` which sets up the signal
handler (using the `sigaction()` call), and `sigint_handler()` which is
the signal handler, itself.

What happens when you run it? If you are in the midst of entering a
string and you hit `^C`, the call to `gets()` fails and sets the global
variable `errno` to `EINTR`. Additionally, `sigint_handler()` is called
and does its routine, so you actually see:

```
Enter a string:
the quick brown fox jum^CAhhh! SIGINT!
fgets: Interrupted system call
```

And then it exits. Hey---what kind of handler is this, if it just exits
anyway?

Well, we have a couple things at play, here. First, you'll notice that
the signal handler was called, because it printed "Ahhh! SIGINT!" But
then `fgets()` returns an error, namely `EINTR`, or "Interrupted system
call". See, some system calls can be interrupted by signals, and when
this happens, they return an error. You might see code like this
(sometimes cited as an excusable use of `goto`):

``` {.c}
restart:
    if (some_system_call() == -1) {
        if (errno == EINTR) goto restart;
        perror("some_system_call");
        exit(1);
    }
```

Instead of using `goto` like that, you might be able to set your
`sa_flags` to include `SA_RESTART`. For example, if we change our
`SIGINT` handler code to look like this:

``` {.c}
    sa.sa_flags = SA_RESTART;
```

Then our run looks more like this:

```
Enter a string:
Hello^CAhhh! SIGINT!
Er, hello!^CAhhh! SIGINT!
This time fer sure!
You entered: This time fer sure!
```

Some system calls are interruptible, and some can be restarted. It's
system dependent.

## What about `signal()`

ANSI C defines a function called `signal()` that can be used to catch
signals. It's not as reliable or as full-featured as `sigaction()`, so
use of `signal()`is generally discouraged.

## Some signals to make you popular

Here is a list of signals you (most likely) have at your disposal:

|Signal|Description|
|:-:|-|
|`SIGABRT`|Process abort signal.|
|`SIGALRM`|Alarm clock.|
|`SIGFPE`|Erroneous arithmetic operation.|
|`SIGHUP`|Hangup.|
|`SIGILL`|Illegal instruction.|
|`SIGINT`|Terminal interrupt signal.|
|`SIGKILL`|Kill (cannot be caught or ignored).|
|`SIGPIPE`|Write on a pipe with no one to read it.|
|`SIGQUIT`|Terminal quit signal.|
|`SIGSEGV`|Invalid memory reference.|
|`SIGTERM`|Termination signal.|
|`SIGUSR1`|User-defined signal 1.|
|`SIGUSR2`|User-defined signal 2.|
|`SIGCHLD`|Child process terminated or stopped.|
|`SIGCONT`|Continue executing, if stopped.|
|`SIGSTOP`|Stop executing (cannot be caught or ignored).|
|`SIGTSTP`|Terminal stop signal.|
|`SIGTTIN`|Background process attempting read.|
|`SIGTTOU`|Background process attempting write.|
|`SIGBUS`|Bus error.|
|`SIGPOLL`|Pollable event.|
|`SIGPROF`|Profiling timer expired.|
|`SIGSYS`|Bad system call.|
|`SIGTRAP`|Trace/breakpoint trap.|
|`SIGURG`|High bandwidth data is available at a socket.|
|`SIGVTALRM`|Virtual timer expired.|
|`SIGXCPU`|CPU time limit exceeded.|
|`SIGXFSZ`|File size limit exceeded.|

Each signal has its own default signal handler, the behavior of which is
defined in your local man pages.

## The Dragons of Reentrancy

If you're busy doing something with global or static data (let's call
that variable `alvin`) and then you get interrupted, what happens if the
handler *also* modifies `alvin`? And then the handler returns and
`alvin` has been messed with behind your back! And your function has no
way to know it! Worse, large data structures might be only _partially_
written to when the handler is called, leading to tearing and horrible
state mashing.

We call these _reentrancy problems_. 

What does that mean? If I might be permitted, I'll just lazily quote
[flw[the Wikipedia article on reentrancy|Reentrancy_(computing)]]:

> Reentrant code is designed to be safe and predictable when multiple
> instances of the same function are called simultaneously or in quick
> succession. A computer program or subroutine is called reentrant if
> multiple invocations can safely run concurrently on multiple
> processors, or if on a single-processor system its execution can be
> interrupted and a new execution of it can be safely started (it can be
> "re-entered"). The interruption could be caused by an internal action
> such as a jump or call [...], or by an external action such as an
> interrupt or signal. 

[flx[Here's a contrived example|sigcount.c]], partial listing below. The
`increment()` function is not reentrant with respect to asynchronous
signals.

Imagine the `increment()` function slowly increasing the global `count`. But
wait! If the signal handler fires at this time, it'll set the `count` to
something that `increment()` isn't expecting! And then things blow up.

(We'll get to `volatile sig_atomic_t` later; for now just assume it's
`int`.)

``` {.c}
volatile sig_atomic_t count;

void handler(int sig)
{
    (void)sig;

    count = 123;
}

void increment(void)
{
    int next_count = count + 1;

    printf("Count is %d, next should be %d\n", count, next_count);

    // Sleep to slow down time to demo the problem
    sleep(2);
    count++;

    if (count == next_count)
        puts("Everything is swell!");
    else
        printf("%d != %d! Aaa! ERROR DOES NOT COMPUTE!\n", count,
            next_count);
}
```

Your takeaway: any time you're reliant on some kind of shared state, you
might have trouble with signals if the signal handler also messes with
that shared state.

[flx[Here's another example using `strtok()`|sigstrtok.c]], which is a
notoriously non-reentrant function.

``` {.c}
void handler(int sig)
{
    (void)sig;

    char x[] = "Hello, world!";
    char *token;

    if ((token = strtok(x, " ")) != NULL) do {
        write(1, "In handler: ", 12);
        write(1, token, strlen(token));
        write(1, "\n", 1);
    } while ((token = strtok(NULL, " ")) != NULL);
}

void tokenizer(void)
{
    char s[] = "The quick brown fox jumped over the lazy dogs";
    char *token;

    if ((token = strtok(s, " ")) != NULL) do {
        printf("In main: %s\n", token);
        // Sleep to slow down time to demo the problem
        sleep(1);
    } while ((token = strtok(NULL, " ")) != NULL);

    puts("Done tokenizing");
}
```

Let's say that two seconds into the `tokenizer()` function, the signal
handler is called. The `handler()` does its own tokenizing of its own
string and prints the tokens[^2baf].

[^2baf]: And it uses `write()` because `printf()` is not reentrant!

If everything went well and sensibly, we'd see this output (but we
don't):

``` {.default}
In main: The
In main: quick
In handler: Hello,
In handler: world!
In main: brown
In main: fox
In main: jumped
In main: over
In main: the
In main: lazy
In main: dogs
Done tokenizing
```

See how the signal occurred, was handled, and `tokenizer()` just
continued where it left off? That would be great, right?

Instead we see this (probably):

``` {.default}
In main: The
In main: quick
In handler: Hello,
In handler: world!
Done tokenizing
```

Where's the rest of it?

Well, `strtok()` maintains some internal state in a `static` variable.
Our `tokenize()` function was expecting the state to be a certain way,
and the signal handler overwrote it, causing `tokenize()` to misbehave.

And this makes `strtok()` non-reentrant (and, by association,
`tokenize()` is therefore also non-reentrant).

The fix is easy, though. We just need a reentrant version of `strtok()`
that doesn't have internal shared state. And we have one in
`strtok_r()`. With that, *we* own the state and we pass it in for
`strtok_r()` to use. Every part of the code that wants a `strtok_r()`
loop will have its own state and no one will step on each others toes.

Here's the code for `strtok_r()` for the `tokenizer()` function (it's
similar for the `handler()` function):

``` {.c}
    char *lasts;

    if ((token = strtok_r(s, " ", &lasts)) != NULL) do {
        printf("In main: %s\n", token);
        // Sleep to slow down time to demo the problem
        sleep(1);
    } while ((token = strtok_r(NULL, " ", &lasts)) != NULL);
```

See how we're tracking our own state in `lasts`? If you replace all the
`strtok()`s with `strtok_r()`s in the demo program, it will work
properly since all the functionality used by both `handler()` and
`tokenizer()` is reentrant.

## Shared Global Data

You cannot safely alter any shared (e.g. global) data, with one notable
exception: variables that are declared to be of storage class and type
`volatile sig_atomic_t`. This is an integer type that holds some range
of values; the C spec guarantees that you'll be at least able to hold 0
to 127, inclusive. But the actual range depends on the system and
whether or not the type is signed. (You can look at `SIG_ATOMIC_MIN` and
`SIG_ATOMIC_MAX` to see what your limits are.)

The spec is very conservative. It basically says you're being very bad
if you do anything with global data other than assign to a variable to
type `volatile sig_atomic_t`. But that's not _super_ true. It's
_probably_ safe to read from the variable, as well, but be advised that
as soon as you read and write to the same variable you're definitely
opening yourself up to some race conditions depending on who else reads
and modifies those values.

Another exception is global shared data that never changes. If you set
up some global variables before the signal handler is installed, and you
never change those values, then the signal handler can read them with
impunity. They can be any type.

Here's an example that handles `SIGUSR1` by setting a global flag, which
is then examined in the main loop to see if the handler was called.
This is [flx[`sigusr.c`|sigusr.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

volatile sig_atomic_t got_usr1;

void sigusr1_handler(int sig)
{
    got_usr1 = 1;
}

int main(void)
{
    struct sigaction sa = {
        .sa_handler = sigusr1_handler,
        .sa_flags = 0, // or SA_RESTART
    };
    sigemptyset(&sa.sa_mask);

    got_usr1 = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (!got_usr1) {
        printf("PID %d: working hard...\n", getpid());
        sleep(1);
    }

    printf("Done in by SIGUSR1!\n");

    return 0;
}
```

Fire it it up in one window, and then use the `kill -USR1` in another
window to kill it. The `sigusr` program conveniently prints out its
process ID so you can pass it to `kill`:

```
$ sigusr
PID 5023: working hard...
PID 5023: working hard...
PID 5023: working hard...
```

Then in the other window, send it the signal `SIGUSR1`:

```
$ kill -USR1 5023
```

And the program should respond:

```
PID 5023: working hard...
PID 5023: working hard...
Done in by SIGUSR1!
```

(And the response should be immediate even if `sleep()` has just been
called---`sleep()` gets interrupted by signals.)

It's a little counter-intuitive to structure the code this way.
Shouldn't the handler have all the handling logic and some some other
piece of code? What if the signal is raised when other code is running
that isn't able to handle it?

That is a bit a of drawback, but structuring the code this way has one
big gain: *bye bye, reentrancy issues!* And that's no bad thing.

### Asynchronous Signal Safety

In light of all the reentrancy pitfalls you might hit, you have to be
careful when you make function calls in your signal handler, and,
indeed, when your handler modifies any global state that other functions
might be using.

Those functions must be _async-signal-safe_, which generally means that
the function doesn't do anything that might cause reentrancy issues.

In general, you're probably not async-signal-safe if you do any of this:

* Modify an non-atomic global variable in your function.
* Read a non-constant global variable that's not atomic.
* Use `static` data in your function or in your handler.
* Call any other function that's not async-signal-safe.

It's pretty limited.

You might be curious, for instance, why my earlier example signal
handler called `write()` to output the message instead of `printf()`.
Well, the answer is that POSIX says that `write()` is async-signal-safe
(so is safe to call from within the handler), while `printf()` is not.

The library functions and system calls that are async-signal-safe and
can be called from within your signal handlers are (breath):

`_Exit()`, `_exit()`, `abort()`, `accept()`, `access()`, `aio_error()`,
`aio_return()`, `aio_suspend()`, `alarm()`, `bind()`, `cfgetispeed()`,
`cfgetospeed()`, `cfsetispeed()`, `cfsetospeed()`, `chdir()`, `chmod()`,
`chown()`, `clock_gettime()`, `close()`, `connect()`, `creat()`,
`dup()`, `dup2()`, `execle()`, `execve()`, `fchmod()`, `fchown()`,
`fcntl()`, `fdatasync()`, `fork()`, `fpathconf()`, `fstat()`, `fsync()`,
`ftruncate()`, `getegid()`, `geteuid()`, `getgid()`, `getgroups()`,
`getpeername()`, `getpgrp()`, `getpid()`, `getppid()`, `getsockname()`,
`getsockopt()`, `getuid()`, `kill()`, `link()`, `listen()`, `lseek()`,
`lstat()`, `mkdir()`, `mkfifo()`, `open()`, `pathconf()`, `pause()`,
`pipe()`, `poll()`, `posix_trace_event()`, `pselect()`, `raise()`,
`read()`, `readlink()`, `recv()`, `recvfrom()`, `recvmsg()`, `rename()`,
`rmdir()`, `select()`, `sem_post()`, `send()`, `sendmsg()`, `sendto()`,
`setgid()`, `setpgid()`, `setsid()`, `setsockopt()`, `setuid()`,
`shutdown()`, `sigaction()`, `sigaddset()`, `sigdelset()`,
`sigemptyset()`, `sigfillset()`, `sigismember()`, `sleep()`, `signal()`,
`sigpause()`, `sigpending()`, `sigprocmask()`, `sigqueue()`, `sigset()`,
`sigsuspend()`, `sockatmark()`, `socket()`, `socketpair()`, `stat()`,
`symlink()`, `sysconf()`, `tcdrain()`, `tcflow()`, `tcflush()`,
`tcgetattr()`, `tcgetpgrp()`, `tcsendbreak()`, `tcsetattr()`,
`tcsetpgrp()`, `time()`, `timer_getoverrun()`, `timer_gettime()`,
`timer_settime()`, `times()`, `umask()`, `uname()`, `unlink()`,
`utime()`, `wait()`, `waitpid()`, and `write()`.

Of course, you can call your own functions from within your signal
handler (as long as they are async-signal-safe and don't call any
non-async-signal-safe functions).

In the next chapter, we'll look at some patterns for safely reacting
when a signal is raised.

## What I have Glossed Over

Nearly all of it. There are tons of flags, realtime signals, mixing
signals with threads, masking signals, `longjmp()` and signals, and
more. I do have another chapter following this one with more in-depth
material, but I could make a whole guide just out of this one topic!

Of course, this is just a "getting started" guide, but in a last-ditch
effort to give you more information, here is a list of man pages with
more information:

Handling signals:

* [flm[`sigaction()`|sigaction.2]]
* [flm[`sigwait()`|sigwait.3]]
* [flm[`sigwaitinfo()`|sigwaitinfo.2]]
* [flm[`sigtimedwait()`|sigtimedwait.2]]
* [flm[`sigsuspend()`|sigsuspend.2]]
* [flm[`sigpending()`|sigpending.2]]

Delivering signals:

* [flm[`kill()`|kill.2]]
* [flm[`raise()`|raise.3]]
* [flm[`sigqueue()`|sigqueue.3]]

Set operations:

* [flm[`sigemptyset()`|sigemptyset.3]]
* [flm[`sigfillset()`|sigfillset.3]]
* [flm[`sigaddset()`|sigaddset.3]]
* [flm[`sigdelset()`|sigdelset.3]]
* [flm[`sigismember()`|sigismember.3]]

Other:

* [flm[`sigprocmask()`|sigprocmask.2]]
* [flm[`sigaltstack()`|sigaltstack.2]]
* [flm[`siginterrupt()`|siginterrupt.3]]
* [flm[`sigsetjmp()`|sigsetjmp.3]]
* [flm[`siglongjmp()`|siglongjmp.3]]
* [flm[`signal()`|signal.2]]

