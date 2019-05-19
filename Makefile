
INCL	= term.h term_table.h
SRC	= main.c term.c
OBJ	= $(SRC:.c=.o)
LIBS	= -lpthread -lecl -lutil -lgc
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

# shelljack: shelljack.c
# 	gcc -o shelljack shelljack.c -Wall -I/usr/local/include -L/usr/local/lib -lpthread -lecl -lutil -lgc
# clean:
# 	rm -f shelljack 

