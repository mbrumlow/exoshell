
INCL	= term.h term_table.h lisp.h log.h test.h util.h proto.h
SRC	= main.c term.c lisp.c log.c test.c util.c proto.c
OBJ	= $(SRC:.c=.o)
LIBS	= -lpthread -lecl -lutil -lgc
EXE	= exoshell

CC	= gcc
CFLAGS	= -Wall -O3
LIBPATH = -L. -L/usr/local/lib
LDFLAGS = -o $(EXE) $(LIBPATH) $(LIBS)
CFDEBUG = -g -DDEBUG $(LDFLAGS)
RM	= rm -f

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS)

$(OBJ): $(INCL)

debug:
	$(CC) $(CFDEBUG) $(SRC)

clean:
	$(RM) $(OBJ) $(EXE) core a.out


