<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- ======================================================= -->
<!-- Memory Mapped Files -->
<!-- ======================================================= -->

# Memory Mapped Files {#mmap}

There comes a time when you want to read and write to and from files so
that the information is shared between processes. Think of it this way:
two processes both open the same file and both read and write from it,
thus sharing the information. The problem is, sometimes it's a pain to
do all those `fseek()`s and stuff to get around. Wouldn't it be easier
if you could just map a section of the file to memory, and get a pointer
to it? Then you could simply use pointer arithmetic to get (and set)
data in the file.

Well, this is exactly what a memory mapped file is. And it's really easy
to use, too. A few simple calls, mixed with a few simple rules, and
you're mapping like a mad-person.

## Mapmake

Before mapping a file to memory, you need to get a file descriptor for
it by using the `open()` system call:

``` {.c}
int fd;

fd = open("mapdemofile", O_RDWR);
```

In this example, we've opened the file for read/write access. You can
open it in whatever mode you want, but it has to match the mode
specified in the `prot` parameter to the `mmap()` call, below.

To memory map a file, you use the `mmap()` system call, which is defined
as follows:

``` {.c}
void *mmap(void *addr, size_t len, int prot,
           int flags, int fildes, off_t off);
```

What a slew of parameters! Here they are, one at a time:

|Parameter|Description|
|--------|----------------------------------------------------------|
|`addr`|This is the address we want the file mapped into. The best way to use this is to set it to `NULL` and let the OS choose it for you. If you tell it to use an address the OS doesn't like (for instance, if it's not a multiple of the virtual memory page size), it'll give you an error.|
|`len`|This parameter is the length of the data we want to map into memory. This can be any length you want. (Aside: if `len` not a multiple of the virtual memory page size, you will get a blocksize that is rounded up to that size. The extra bytes will be 0, and any changes you make to them will not modify the file.)|
|`prot`|The "protection" argument allows you to specify what kind of access this process has to the memory mapped region. This can be a bitwise-ORd mixture of the following values: `PROT_READ`, `PROT_WRITE`, and `PROT_EXEC`, for read, write, and execute permissions, respectively. The value specified here must be equivalent to or a subset of the modes specified in the `open()` system call that is used to get the file descriptor.|
|`flags`|These are just miscellaneous flags that can be set for the system call. You'll want to set it to `MAP_SHARED` if you're planning to share your changes to the file with other processes, or `MAP_PRIVATE` otherwise. If you set it to the latter, your process will get a copy of the mapped region, so any changes you make to it will not be reflected in the original file---thus, other processes will not be able to see them. We won't talk about `MAP_PRIVATE` here at all, since it doesn't have much to do with IPC.|
|`fildes`|This is where you put that file descriptor you opened earlier.|
|`off`|This is the offset in the file that you want to start mapping from. A restriction: this _must_ be a multiple of the virtual memory page size. This page size can be obtained with a call to `getpagesize()`. Note that 32-bit systems may support files with sizes that cannot be expressed by 32-bit unsigned integers, so this type is often a 64-bit type on such systems.|

As for return values, as you might have guessed, `mmap()` returns
`MAP_FAILED` on error (the value `-1` properly cast to be compared), and
sets `errno`. Otherwise, it returns a pointer to the start of the mapped
data.

Anyway, without any further ado, we'll do a short demo that maps the
second "page" of a file into memory. First we'll `open()` it to get the
file descriptor, then we'll use `getpagesize()` to get the size of a
virtual memory page and use this value for both the `len` and the `off`.
In this way, we'll start mapping at the second page, and map for one
page's length. (On my Linux box, the page size is 4K.)

``` {.c}
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

int fd, pagesize;
char *data;

fd = open("foo", O_RDONLY);
pagesize = getpagesize();
data = mmap((void*)0, pagesize, PROT_READ, MAP_SHARED, fd, pagesize);
```

Once this code stretch has run, you can access the first byte of the
mapped section of file using `data[0]`. Notice there's a lot of type
conversion going on here. For instance, `mmap()` returns `void*`, but we
treat it as a `char*`.

Also notice that we've mapped the file `PROT_READ` so we have read-only
access. Any attempt to write to the data (`data[0] = 'B'`, for example)
will cause a segmentation violation. Open the file `O_RDWR` with `prot`
set to `PROT_READ|PROT_WRITE` if you want read-write access to the data.

## Unmapping the file

There is, of course, a `munmap()` function to un-memory map a file:

``` {.c}
int munmap(void *addr, size_t len);
```

This simply unmaps the region pointed to by `addr` (returned from
`mmap()`) with length `len` (same as the `len` passed to `mmap()`).
`munmap()` returns `-1` on error and sets the `errno` variable.

Once you've unmapped a file, any attempts to access the data through the
old pointer will result in a segmentation fault. You have been warned!

A final note: the file will automatically unmap if your program exits,
of course.

## Concurrency, again?!

If you have multiple processes manipulating the data in the same file
concurrently, you could be in for troubles. You might have to [lock the
file](#flocking) or use [semaphores](#semaphores) to regulate access to
the file while a process messes with it. Look at the [Shared
Memory](#shmcon) document for a (very little bit) more concurrency
information.

## A simple sample

Well, it's code time again. I've got here a demo program that maps its
own source to memory and prints the byte that's found at whatever offset
you specify on the command line.

The program restricts the offsets you can specify to the range 0 through
the file length. The file length is obtained through a call to `stat()`
which you might not have seen before. It returns a structure full of
file info, one field of which is the size in bytes. Easy enough.

Here is the source for [flx[`mmapdemo.c`|mmapdemo.c]]:

``` {.c .numberLines}
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int fd;
    off_t offset;
    char *data;
    struct stat sbuf;

    if (argc != 2) {
            fprintf(stderr, "usage: mmapdemo offset\n");
            exit(1);
    }

    if ((fd = open("mmapdemo.c", O_RDONLY)) == -1) {
            perror("open");
            exit(1);
    }

    if (stat("mmapdemo.c", &sbuf) == -1) {
            perror("stat");
            exit(1);
    }


    offset = atoi(argv[1]);
    if (offset < 0 || offset > sbuf.st_size-1) {
        fprintf(stderr, "mmapdemo: offset must be in the range 0-%d\n", \
                                                          sbuf.st_size-1);
        exit(1);
    }
    
    data = mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("byte at offset %ld is '%c'\n", offset, data[offset]);

    return 0;
}
```

That's all there is to it. Compile that sucker up and run it with some
command line like:

``` {.default}
$ mmapdemo 30
byte at offset 30 is 'e'
```

I'll leave it up to you to write some really cool programs using this
system call.

## Observations on memory mapping

I would be remiss if I didn't point out a few interesting aspects of
using mapped files on Linux. First, the memory that the operating system
allocates to use as the storage for the mapped file data is _the same
memory_ used to perform file buffering operations when other processes
perform `read()` and `write()` operations! While `read()`s and
`write()`s are guaranteed atomic by POSIX up to a certain size, that
goes out the window when some processes bypass the POSIX functions
entirely!

Second, because we're bypassing those POSIX functions, we can read and
write the buffer contents without regard to record locking that might be
applied to the file descriptor (as discussed in a previous section).
Normally, this isn't a big deal---who's going to use memory mapped files
in one application while using record locking in another, when both
access the same file? If the file is documented to require record
locking, then all applications should use it. That said, there's nothing
stopping an application from using the read and write locking we
discussed previously immediately before updating the memory that belongs
to the mapped file.

Third, because we're bypassing those POSIX functions (do I sound like a
broken record yet?), the system is not capable of providing meaningful
readahead or writebehind strategies. As of this writing, Linux kernel
versions 4.x and later _do_ implement an algorithm that detects when two
adjacent page faults occur within a memory mapped file, and it performs
a minimal amount of readahead (just two pages, compared to the readahead
configurable at the file system layer, which can be upwards of 256KB).
There is no writebehind whatsoever, as there's no practical way to
detect when adjacent pages are written to under current hardware
configurations.

Last, given all of the above, there are still very compelling reasons to
use memory mapped files. The primary one being that such files are, by
definition, "persistent storage", meaning applications do not have to
create lengthy `load()`/`save()` functions for their data if they use
memory mapped files. However, any binary data will be written in a
platform dependent manner (such as endianness) so those files are likely
not portable.

## Summary

Memory mapped files can be very useful, especially on systems that don't
support shared memory segments. In fact, the two are very similar in
most respects. (Memory mapped files are committed to disk, too, so this
could even be an advantage, yes?)  With file locking or semaphores, data
in a memory mapped file can easily be shared between multiple processes.
