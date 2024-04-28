CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=gnu99
DEPS=skitour.h

default: proj2

proj2: proj2.o skitour.o
	$(CC) $(CFLAGS) -o proj2 proj2.o skitour.o

proj2.o: proj2.c skitour.h
	$(CC) $(CFLAGS) -c proj2.c

skitour.o: skitour.c skitour.h
	$(CC) $(CFLAGS) -c skitour.c

clean:
	$(RM) proj2 *.o *.out

