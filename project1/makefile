OBJS = observer.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

a : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o a.out

observer.o: observer.cpp
	$(CC) $(CFLAGS) observer.cpp

clean:
	rm -rf *~
	rm -rf *.o
	rm -rf *.out