CC= gcc
CFLAGS= -O2 -Wall $(MYCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS=

#MYCFLAGS=-ansi -std=c89 -pedantic
MYLDFLAGS=
MYLIBS=

all: tty2tty rx

tty2tty: tty2tty.o
	$(CC)  -o $@ $^ 

rx: rx.o xmodem.o
	$(CC)  -o $@ $^ 
	
clean:
	$(RM) tty2tty rx *.o

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all clean  
