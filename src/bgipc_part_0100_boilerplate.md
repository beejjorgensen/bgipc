<!-- Beej's guide to IPC

# vim: ts=4:sw=4:nosi:et:tw=72
-->

<!-- No hyphenation -->
[nh[fork]]

<!-- ======================================================= -->
<!-- Introduction -->
<!-- ======================================================= -->

# Intro

You know what's easy? `fork()` is easy. You can fork off new processes
all day and have them deal with individual chunks of a problem in
parallel. Of course, its easiest if the processes don't have to
communicate with one another while they're running and can just sit
there doing their own thing.

However, when you start `fork()`'ing processes, you immediately start to
think of the neat multi-user things you could do if the processes could
talk to each other easily. So you try making a global array and then
`fork()`'ing to see if it is shared. (That is, see if both the child
and parent process use the same array.) Soon, of course, you find that
the child process has its own copy of the array and the parent is
oblivious to whatever changes the child makes to it.

How do you get these guys to talk to one another, share data structures,
and be generally amicable? This document discusses several methods of
_Interprocess Communication_ (IPC) that can accomplish this, some of
which are better suited to certain tasks than others.

<!-- ======================================================= -->
<!-- Audience -->
<!-- ======================================================= -->

## Audience

If you know C or C++ and are pretty good using a Unix environment (or
other POSIXey environment that supports these system calls) these
documents are for you. If you aren't that good, well, don't sweat
it---you'll be able to figure it out. I make the assumption, however,
that you have a fair smattering of C programming experience.

As with [flbg[Beej's Guide to Network Programming Using Internet
Sockets|bgnet]], these documents are meant to springboard the
aforementioned user into the realm of IPC by delivering a concise
overview of various IPC techniques. This is not the definitive set of
documents that cover this subject, by any means. Like I said, it is
designed to simply give you a foothold in this, the exciting world of
IPC.

<!-- ======================================================= -->
<!-- Platform and Compiler -->
<!-- ======================================================= -->
## Platform and Compiler

The examples in this document were compiled under Linux using `gcc`.
They should compile anywhere a good Unix compiler is available.

<!-- ======================================================= -->
<!-- Homepage -->
<!-- ======================================================= -->
## Official Homepage

This official location of this document is
[flbg[`https://beej.us/guide/bgipc/`|bgipc]].

<!-- ======================================================= -->
<!-- Email policy -->
<!-- ======================================================= -->
## Email Policy

I'm generally available to help out with email questions so feel free to
write in, but I can't guarantee a response. I lead a pretty busy life
and there are times when I just can't answer a question you have. When
that's the case, I usually just delete the message. It's nothing
personal; I just won't ever have the time to give the detailed answer
you require.

As a rule, the more complex the question, the less likely I am to
respond. If you can narrow down your question before mailing it and be
sure to include any pertinent information (like platform, compiler,
error messages you're getting, and anything else you think might help me
troubleshoot), you're much more likely to get a response.

If you don't get a response, hack on it some more, try to find the
answer, and if it's still elusive, then write me again with the
information you've found and hopefully it will be enough for me to help
out.

Now that I've badgered you about how to write and not write me, I'd just
like to let you know that I _fully_ appreciate all the praise the guide
has received over the years. It's a real morale boost, and it gladdens
me to hear that it is being used for good! `:-)` Thank you!

<!-- ======================================================= -->
<!-- Mirroring -->
<!-- ======================================================= -->

## Mirroring

You are more than welcome to mirror this site, whether publicly or
privately. If you publicly mirror the site and want me to link to it
from the main page, drop me a line at
[`beej@beej.us`](mailto:beej@beej.us).

<!-- ======================================================= -->
<!-- Translators -->
<!-- ======================================================= -->

## Note for Translators

If you want to translate the guide into another language, write me at
[`beej@beej.us`] and I'll link to your translation from the main page.
Feel free to add your name and contact info to the translation.

Please note the license restrictions in the Copyright and Distribution
section, below.

<!-- ======================================================= -->
<!-- Copyright -->
<!-- ======================================================= -->
## Copyright and Distribution

Beej's Guide to Network Programming is Copyright © 2021 Brian "Beej
Jorgensen" Hall.

With specific exceptions for source code and translations, below, this
work is licensed under the Creative Commons Attribution-Noncommercial-No
Derivative Works 3.0 License. To view a copy of this license, visit
[`https://creativecommons.org/licenses/by-nc-nd/3.0/`](https://creativecommons.org/licenses/by-nc-nd/3.0/)
or send a letter to Creative Commons, 171 Second Street, Suite 300, San
Francisco, California, 94105, USA.

One specific exception to the "No Derivative Works" portion of the
license is as follows: this guide may be freely translated into any
language, provided the translation is accurate, and the guide is
reprinted in its entirety. The same license restrictions apply to the
translation as to the original guide. The translation may also include
the name and contact information for the translator.

The C source code presented in this document is hereby granted to the
public domain, and is completely free of any license restriction.

Educators are freely encouraged to recommend or supply copies of this
guide to their students.

Contact [`beej@beej.us`](mailto:beej@beej.us) for more information.

