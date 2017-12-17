# to be included from singlepage/Makefile and multipage/Makefile

PACKAGE=bgipc
BASE=../../..
SRCDIR=$(BASE)/src
SRC=$(SRCDIR)/$(PACKAGE).xml
CSS=$(SRCDIR)/$(PACKAGE).css
VALIDFILE=$(BASE)/$(PACKAGE).valid
BINPATH=$(BASE)/bin
LIBPATH=$(BASE)/lib
IMGPATH=$(SRCDIR)/images
IMGS=pipe1-96-4.149.png
HEADER="Beej's Guide to Unix IPC"

PYTHONPATH=../lib:$(LIBPATH)
export PYTHONPATH

