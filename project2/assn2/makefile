OBJS = main.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

a : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o a.out

main.o: main.cc assn2.h
	$(CC) $(LFLAGS) -c main.cc

clean:
	rm -rf *~
	rm -rf *.o
	rm -rf *.out