AM_CFLAGS  = -I$(top_srcdir)/src
AM_CFLAGS += -DDATADIR='"$(datadir)/exoshell"'

AM_LDFLAGS = -lpthread -lecl -lutil -lgc

bin_PROGRAMS = exoshell
exoshell_SOURCES = main.c term.c lisp.c log.c test.c util.c proto.c
