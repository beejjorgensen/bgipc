<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Unix Sockets -->
<!-- ======================================================= -->

# Unix Sockets {#unixsock}

Remember <link dest="fifos">FIFOs</link>? Remember how they can
only send data in one direction, just like <link
dest="pipes">Pipes</link>? Wouldn't it be grand if you could send
data in both directions like you can with a socket?

Well, hope no longer, because the answer is here: Unix Domain Sockets!
In case you're still wondering what a socket is, well, it's a two-way
communications pipe, which can be used to communicate in a wide variety
of _domains_. One of the most common domains sockets communicate
over is the Internet, but we won't discuss that here. We will, however,
be talking about sockets in the Unix domain; that is, sockets that can
be used between processes on the same Unix system.

Unix sockets use many of the same function calls that Internet
sockets do, and I won't be describing all of the calls I use in detail
within this document. If the description of a certain call is too vague
(or if you just want to learn more about Internet sockets anyway), I
arbitrarily suggest <booktitle><ulink url="&bgneturl;">Beej's Guide to
Network Programming using Internet Sockets</ulink></booktitle>. I know
the author personally.

<sect2 id="unixsockover">
<title>Overview</title>

Like I said before, Unix sockets are just like two-way FIFOs.
However, all data communication will be taking place through the sockets
interface, instead of through the file interface. Although Unix sockets
are a special file in the file system (just like FIFOs), you won't be
using `open()` and `read()`---you'll be using
`socket()`, `bind()`, `recv()`,
etc.

When programming with sockets, you'll usually create server and
client programs. The server will sit listening for incoming connections
from clients and handle them. This is very similar to the situation
that exists with Internet sockets, but with some fine differences.

For instance, when describing which Unix socket you want to use (that
is, the path to the special file that is the socket), you use a
`struct sockaddr_un`, which has the following
fields:

<code>struct sockaddr_un {
    unsigned short sun_family;  /* AF_UNIX */
    char sun_path[108];
}</code>

This is the structure you will be passing to the `bind()`
function, which associates a socket descriptor (a file descriptor) with
a certain file (the name for which is in the `sun_path`
field).

</sect2>

<sect2 id="unixsockserv">
<title>What to do to be a Server</title>

Without going into too much detail, I'll outline the steps a server
program usually has to go through to do it's thing. While I'm at it,
I'll be trying to implement an "echo server" which just echos back
everything it gets on the socket.

Here are the server steps:

<numlist>
<li><b>Call `socket()`:</b>  A call to
`socket()` with the proper arguments creates the Unix
socket:

<code>unsigned int s, s2;
struct sockaddr_un remote, local = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
};
int len;

s = socket(_AF_UNIX_, SOCK_STREAM, 0);</code>

The second argument, `SOCK_STREAM`, tells
`socket()` to create a stream socket. Yes, datagram sockets
(`SOCK_DGRAM`) are supported in the Unix domain, but I'm
only going to cover stream sockets here. For the curious, see <ulink
url="&bgneturl;">Beej's Guide to Network Programming</ulink> for a good
description of unconnected datagram sockets that applies perfectly well
to Unix sockets. The only thing that changes is that you're now using a
`struct sockaddr_<b>un</b>` instead of a
`struct sockaddr_<b>in</b>`.

One more note: all these calls return `-1` on error and
set the global variable `errno` to reflect whatever went wrong.
Be sure to do your error checking.
</li>

<li><b>Call `bind()`:</b>  You got a socket descriptor
from the call to `socket()`, now you want to bind that to an
address in the Unix domain. (That address, as I said before, is a
special file on disk.)

<code>strcpy(local.sun_path, "/home/beej/mysocket");
unlink(local.sun_path);
len = strlen(local.sun_path) + sizeof(local.sun_family);
bind(s, (struct sockaddr *)&local, len);</code>

This associates the socket descriptor "`s`" with the Unix
socket address "`/home/beej/mysocket`". Notice that we called
`unlink()` before `bind()` to remove the socket if
it already exists. You will get an `EINVAL` error if the file is
already there.
</li>

<li><b>Call `listen()`:</b>  This instructs the socket to
listen for incoming connections from client programs:

<code>listen(s, 5);</code>

The second argument, `5`, is the number of incoming
connections that can be queued before you call `accept()`,
below. If there are this many connections waiting to be accepted,
additional clients will generate the error
`ECONNREFUSED`.

</li>

<li><b>Call `accept()`:</b>  This will accept a connection
from a client. This function returns _another socket
descriptor_! The old descriptor is still listening for new
connections, but this new one is connected to the client:

<code>len = sizeof(remote);
s2 = accept(s, &remote, &len);</code>

When `accept()` returns, the `remote` variable
will be filled with the remote side's `struct
sockaddr_un`, and `len` will be set to its length.
The descriptor `s2` is connected to the client, and is ready
for `send()` and `recv()`, as described in the
<ulink url="&bgneturl;">Network Programming Guide</ulink>.
</li>

<li><b>Handle the connection and loop back to
`accept()`:</b> Usually you'll want to communicate to the
client here (we'll just echo back everything it sends us), close the
connection, then `accept()` a new one.

<code>while (len = recv(s2, &buf, 100, 0), len > 0)
    send(s2, &buf, len, 0);

/* loop back to accept() from here */</code>

<!-- if <code> is the last element in an <li>, FOP pukes. Why?? -->

</li>

<li><b>Close the connection:</b>  You can close the connection either
by calling `close()`, or by calling <ulink
url="&redir;shutdownman">`shutdown()`</ulink>.

</li>

</numlist>

With all that said, here is some source for an echoing server, <ulink
url="&samplepre;echos.c">`echos.c`</ulink>. All it does is wait for a
connection on a Unix socket (named, in this case, "echo_socket").

<code><![CDATA[#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "echo_socket"

int main(void)
{
    int s, s2, len;
    struct sockaddr_un remote, local = {
            .sun_family = AF_UNIX,
            // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
    }

    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
            perror("bind");
            exit(1);
    }

    if (listen(s, 5) == -1) {
            perror("listen");
            exit(1);
    }

    for(;;) {
            int done, n;
            printf("Waiting for a connection...\n");
            socklen_t slen = sizeof(remote);
            if ((s2 = accept(s, (struct sockaddr *)&remote, &slen)) == -1) {
                    perror("accept");
                    exit(1);
            }

            printf("Connected.\n");

            done = 0;
            do {
                    n = recv(s2, str, sizeof(str), 0);
                    if (n <= 0) {
                            if (n < 0) perror("recv");
                            done = 1;
                    }

                    if (!done) 
                            if (send(s2, str, n, 0) < 0) {
                                    perror("send");
                                    done = 1;
                            }
            } while (!done);

            close(s2);
    }

    return 0;
}]]></code>

As you can see, all the aforementioned steps are included in this
program: call `socket()`, call `bind()`, call
`listen()`, call `accept()`, and do some network
`send()`s and `recv()`s.

</sect2>

<sect2 id="sockclient">
<title>What to do to be a client</title>

There needs to be a program to talk to the above server, right?
Except with the client, it's a lot easier because you don't have to do
any pesky `listen()`ing or `accept()`ing. Here
are the steps:

<numlist>
<li>Call `socket()` to get a Unix domain socket to communicate
through.</li>

<li>Set up a `struct sockaddr_un` with the
remote address (where the server is listening) and call
`connect()` with that as an argument</li>

<li>Assuming no errors, you're connected to the remote side! Use
`send()` and `recv()` to your heart's
content!</li>

</numlist>

How about code to talk to the echo server, above? No sweat, friends,
here is <ulink
url="&samplepre;echoc.c">`echoc.c`</ulink>:

<code><![CDATA[#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "echo_socket"

int main(void)
{
    int s, len;
    struct sockaddr_un remote = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");

    /* size in fgets() includes the null byte */
    while(printf("> "), fgets(str, sizeof(str), stdin), !feof(stdin)) {
        if (send(s, str, strlen(str)+1, 0) == -1) {
            perror("send");
            exit(1);
        }

        if ((len=recv(s, str, sizeof(str)-1, 0)) > 0) {
            str[len] = '\0';
            printf("echo> %s", str);
        } else {
            if (len < 0) perror("recv");
            else printf("Server closed connection\n");
            exit(1);
        }
    }

    close(s);

    return 0;
}]]></code>

In the client code, of course you'll notice that there are only a few
system calls used to set things up: `socket()` and
`connect()`. Since the client isn't going to be
`accept()`ing any incoming connections, there's no need for
it to `listen()`. Of course, the client still uses
`send()` and `recv()` for transferring data. That
about sums it up.

</sect2>

<sect2 id="socketpair">
<title>`socketpair()`---quick full-duplex pipes</title>

What if you wanted a <link dest="pipes">`pipe()`</link>,
but you wanted to use a single pipe to send and recieve data from
_both sides_? Since pipes are unidirectional (with exceptions
in SYSV), you can't do it! There is a solution, though: use a Unix
domain socket, since they can handle bi-directional data.

What a pain, though! Setting up all that code with
`listen()` and `connect()` and all that just to
pass data both ways! But guess what! You don't have to!

That's right, there's a beauty of a system call known as
`socketpair()` this is nice enough to return to you a pair of
_already connected sockets_! No extra work is needed on your
part; you can immediately use these socket descriptors for interprocess
communication.

For instance, lets set up two processes. The first sends a
`char` to the second, and the second changes the character to
uppercase and returns it. Here is some simple code to do just that,
called `<ulink url="&samplepre;spair.c">spair.c</ulink>`
(with no error checking for clarity):

<code><![CDATA[#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(void)
{
    int sv[2]; /* the pair of socket descriptors */
    char buf; /* for data exchange between processes */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
            perror("socketpair");
            exit(1);
    }

    if (!fork()) {  /* child */
            read(sv[1], &buf, 1);
            printf("child: read '%c'\n", buf);
            buf = toupper(buf);  /* make it uppercase */
            write(sv[1], &buf, 1);
            printf("child: sent '%c'\n", buf);

    } else { /* parent */
            write(sv[0], "b", 1);
            printf("parent: sent 'b'\n");
            read(sv[0], &buf, 1);
            printf("parent: read '%c'\n", buf);
            wait(NULL); /* wait for child to die */
    }

    return 0;
}]]></code>

Sure, it's an expensive way to change a character to uppercase, but
it's the fact that you have simple communication going on here that
really matters.

One more thing to notice is that `socketpair()` takes both
a domain (`AF_UNIX`) and socket type
(`SOCK_STREAM`). These can be any legal values at all,
depending on which routines in the kernel you want to handle your code,
and whether you want stream or datagram sockets. I chose
`AF_UNIX` sockets because this is a Unix sockets document
and they're a bit faster than `AF_INET` sockets, I
hear.

Finally, you might be curious as to why I'm using
`write()` and `read()` instead of
`send()` and `recv()`. Well, in short, I was
being lazy. See, by using these system calls, I don't have to enter the
`flags` argument that `send()` and
`recv()` use, and I always set it to zero anyway. Of course,
socket descriptors are just file descriptors like any other, so they
respond just fine to many file manipulation system calls.

</sect2>

</sect1> <!-- Unix Sockets -->

<!-- ======================================================= -->
<!-- References -->
<!-- ======================================================= -->

<sect1 id="references">
<title>More IPC Resources</title>

<sect2 id="refbooks">
<title>Books</title>

Here are some books that describe some of the procedures I've
discussed in this guide, as well as Unix details in specific:

<referenceset>

<reference><title>Unix Network Programming, volumes 1-2</title> by W.
Richard Stevens. Published by Prentice Hall. ISBNs for volumes 1-2:
<ulink url="&redir;unixnet1">0131411551</ulink>,
<ulink url="&redir;unixnet2">0130810819</ulink>.
</reference>

<reference><title>Advanced Programming in the UNIX
Environment</title> by W. Richard Stevens. Published by Addison
Wesley. ISBN
<ulink url="&redir;advunix">0201433079</ulink>.
</reference>

<reference>Bach, Maurice J. <title>The Design of the UNIX Operating
System</title>. New Jersey: Prentice-Hall, 1986. ISBN <ulink
url="&redir;unixdesign">0132017997</ulink>.</reference>

</referenceset>

</sect2>

<sect2 id="onlineref">
<title>Other online documentation</title>

<referenceset>

<reference><title><ulink url="&unpurl;">UNIX Network Programming Volume
2 home page</ulink></title>---includes source code from Stevens'
superfine book</reference>

<reference><title><ulink url="&lpgipc;">The Linux Programmer's
Guide</ulink></title>---in-depth section on IPC</reference>

<reference><title><ulink url="&davesysurl;">UNIX System Calls and
Subroutines using C</ulink></title>---contains modest IPC
information</reference>

<reference><title><ulink url="&linuxkernelipc;">The Linux
Kernel</ulink></title>---how the Linux kernel implements
IPC</reference>

</referenceset>

</sect2>

<!-- ======================================================= -->
<!-- Linux man pages -->
<!-- ======================================================= -->

<sect2 id="manpages">
<title>Linux man pages</title>

There are Linux manual pages. If you run another flavor of Unix,
please look at your own man pages, as these might not work on your
system.

<ulink url="&manpre;2/accept.2&manpost;">`accept()`</ulink>,
<ulink url="&manpre;2/bind.2&manpost;">`bind()`</ulink>,
<ulink url="&manpre;2/connect.2&manpost;">`connect()`</ulink>,
<ulink url="&manpre;2/dup.2&manpost;">`dup()`</ulink>,
<ulink url="&manpre;2/exec.2&manpost;">`exec()`</ulink>,
<ulink url="&manpre;2/exit.2&manpost;">`exit()`</ulink>,
<ulink url="&manpre;2/fcntl.2&manpost;">`fcntl()`</ulink>,
<ulink url="&manpre;3/fileno.3&manpost;">`fileno()`</ulink>,
<ulink url="&manpre;2/fork.2&manpost;">`fork()`</ulink>,
<ulink url="&manpre;3/ftok.3&manpost;">`ftok()`</ulink>,
<ulink url="&manpre;2/getpagesize.2&manpost;">`getpagesize()`</ulink>,
<ulink url="&manpre;8/ipcrm.8&manpost;">`ipcrm`</ulink>,
<ulink url="&manpre;8/ipcs.8&manpost;">`ipcs`</ulink>,
<ulink url="&manpre;1/kill.1&manpost;">`kill`</ulink>,
<ulink url="&manpre;2/kill.2&manpost;">`kill()`</ulink>,
<ulink url="&manpre;2/listen.2&manpost;">`listen()`</ulink>,
<ulink url="&manpre;2/lockf.2&manpost;">`lockf()`</ulink>,
<ulink url="&manpre;2/lseek.2&manpost;">`lseek()`</ulink> (for the `l_whence` field in `struct flock`),
<ulink url="&manpre;1/mknod.1&manpost;">`mknod`</ulink>,
<ulink url="&manpre;2/mknod.2&manpost;">`mknod()`</ulink>,
<ulink url="&manpre;2/mmap.2&manpost;">`mmap()`</ulink>,
<ulink url="&manpre;2/msgctl.2&manpost;">`msgctl()`</ulink>,
<ulink url="&manpre;2/msgget.2&manpost;">`msgget()`</ulink>,
<ulink url="&manpre;2/msgsnd.2&manpost;">`msgsnd()`</ulink>,
<ulink url="&manpre;2/munmap.2&manpost;">`munmap()`</ulink>,
<ulink url="&manpre;2/open.2&manpost;">`open()`</ulink>,
<ulink url="&manpre;2/pipe.2&manpost;">`pipe()`</ulink>,
<ulink url="&manpre;1/ps.1&manpost;">`ps`</ulink>,
<ulink url="&manpre;3/raise.3&manpost;">`raise()`</ulink>,
<ulink url="&manpre;2/read.2&manpost;">`read()`</ulink>,
<ulink url="&manpre;2/recv.2&manpost;">`recv()`</ulink>,
<ulink url="&manpre;2/semctl.2&manpost;">`semctl()`</ulink>,
<ulink url="&manpre;2/semget.2&manpost;">`semget()`</ulink>,
<ulink url="&manpre;2/semop.2&manpost;">`semop()`</ulink>,
<ulink url="&manpre;2/send.2&manpost;">`send()`</ulink>,
<ulink url="&manpre;2/shmat.2&manpost;">`shmat()`</ulink>,
<ulink url="&manpre;2/shmctl.2&manpost;">`shmctl()`</ulink>,
<ulink url="&manpre;2/shmdt.2&manpost;">`shmdt()`</ulink>,
<ulink url="&manpre;2/shmget.2&manpost;">`shmget()`</ulink>,
<ulink url="&manpre;2/sigaction.2&manpost;">`sigaction()`</ulink>,
<ulink url="&manpre;2/signal.2&manpost;">`signal()`</ulink>,
<ulink url="&manpre;7/signal.7&manpost;">signals</ulink>,
<ulink url="&manpre;2/sigpending.2&manpost;">`sigpending()`</ulink>,
<ulink url="&manpre;2/sigprocmask.2&manpost;">`sigprocmask()`</ulink>,
<ulink url="&manpre;2/sigsetops.2&manpost;">sigsetops</ulink>,
<ulink url="&manpre;2/sigsuspend.2&manpost;">`sigsuspend()`</ulink>,
<ulink url="&manpre;2/socket.2&manpost;">`socket()`</ulink>,
<ulink url="&manpre;2/socketpair.2&manpost;">`socketpair()`</ulink>,
<ulink url="&manpre;2/stat.2&manpost;">`stat()`</ulink>,
<ulink url="&manpre;2/wait.2&manpost;">`wait()`</ulink>,
<ulink url="&manpre;2/waitpid.2&manpost;">`waitpid()`</ulink>,
<ulink url="&manpre;2/write.2&manpost;">`write()`</ulink>.

</sect2>

</sect1> <!-- References -->

</guide>

