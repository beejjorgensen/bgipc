<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- References -->
<!-- ======================================================= -->

# More IPC Resources {#references}

## Books

Here are some books that describe some of the procedures I've discussed
in this guide, as well as Unix details in specific:

Bach, Maurice J. _The Design of the UNIX Operating System_. Published by
Prentice-Hall, 1986. ISBN [flr[0132017997|unixdesign]].

W. Richard Stevens. _Unix Network Programming, volumes 1-2_. Published
by Prentice Hall. ISBNs for volumes 1-2: [flr[0131411551|unixnet1]],
[flr[0130810819|unixnet2]].

W. Richard Stevens. _Advanced Programming in the UNIX Environment_.
Published by Addison Wesley. ISBN [flr[0201433079|advunix]].

## Other online documentation

[fl[**UNIX Network Programming Volume 2 home
page**|http://www.kohala.com/start/unpv22e/unpv22e.html]]---includes
source code from Stevens' superfine book.

[fl[**The Linux Programmer's
Guide**|http://tldp.org/LDP/lpg/node7.html]]---in-depth section on IPC.

[fl[**UNIX System Calls and Subroutines using
C**|https://users.cs.cf.ac.uk/Dave.Marshall/C/]]---contains modest IPC
information.

[fl[**The Linux Kernel**|https://tldp.org/LDP/tlk/ipc/ipc.html]]---how
the Linux kernel implements IPC.

<!-- ======================================================= -->
<!-- Linux man pages -->
<!-- ======================================================= -->

## Linux man pages

There are Linux manual pages. If you run another flavor of Unix,
please look at your own man pages, as these might not work on your
system.

* [flm[`accept()`|accept.2]],
* [flm[`bind()`|bind.2]],
* [flm[`connect()`|connect.2]],
* [flm[`dup()`|dup.2]],
* [flm[`exec()`|exec.2]],
* [flm[`exit()`|exit.2]],
* [flm[`fcntl()`|fcntl.2]],
* [flm[`fileno()`|fileno.3]],
* [flm[`fork()`|fork.2]],
* [flm[`ftok()`|ftok.3]],
* [flm[`getpagesize()`|getpagesize.2]],
* [flm[`ipcrm`|ipcrm.8]],
* [flm[`ipcs`|ipcs.8]],
* [flm[`kill`|kill.1]],
* [flm[`kill()`|kill.2]],
* [flm[`listen()`|listen.2]],
* [flm[`lockf()`|lockf.2]],
* [flm[`lseek()`|lseek.2]] (for the `l_whence` field in `struct flock`),
* [flm[`mknod`|mknod.1]],
* [flm[`mknod()`|mknod.2]],
* [flm[`mmap()`|mmap.2]],
* [flm[`msgctl()`|msgctl.2]],
* [flm[`msgget()`|msgget.2]],
* [flm[`msgsnd()`|msgsnd.2]],
* [flm[`munmap()`|munmap.2]],
* [flm[`open()`|open.2]],
* [flm[`pipe()`|pipe.2]],
* [flm[`ps`|ps.1]],
* [flm[`raise()`|raise.3]],
* [flm[`read()`|read.2]],
* [flm[`recv()`|recv.2]],
* [flm[`semctl()`|semctl.2]],
* [flm[`semget()`|semget.2]],
* [flm[`semop()`|semop.2]],
* [flm[`send()`|send.2]],
* [flm[`shmat()`|shmat.2]],
* [flm[`shmctl()`|shmctl.2]],
* [flm[`shmdt()`|shmdt.2]],
* [flm[`shmget()`|shmget.2]],
* [flm[`sigaction()`|sigaction.2]],
* [flm[`signal()`|signal.2]],
* [flm[signals|signal.7]],
* [flm[`sigpending()`|sigpending.2]],
* [flm[`sigprocmask()`|sigprocmask.2]],
* [flm[sigsetops|sigsetopts.2]],
* [flm[`sigsuspend()`|sigsuspend.2]],
* [flm[`socket()`|socket.2]],
* [flm[`socketpair()`|socketpair.2]],
* [flm[`stat()`|stat.2]],
* [flm[`wait()`|wait.2]],
* [flm[`waitpid()`|waitpid.2]],
* [flm[`write()`|write.2]].

