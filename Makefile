
INCL	= term.h term_table.h lisp.h log.h test.h
SRC	= main.c term.c lisp.c log.c test.c
OBJ	= $(SRC:.c=.o)
LIBS	= -lpthread -lecl -lutil -lgc -lreadline
EXE	= exoshell

CC	= gcc
CFLAGS	= -Wall
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


