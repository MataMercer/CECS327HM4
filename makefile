CC1=gcc -O3 -Wall -pthread

CFLAGS = -g -c -Wall -pthread
#CFLAGS = -ansi -c -Wall -pedantic
all: TCP_Client p2pregistry UDP_Client

A = TCP_Client.o
B = p2pregistry.o
C = UDP_Client.o

TCP_Client: $A
	$(CC1) -o TCP_Client $A

TCP_Client.o: TCP_Client.c
	$(CC1) -c TCP_Client.c

p2pregistry: $B
	$(CC1) -o p2pregistry $B

p2pregistry.o: p2pregistry.c
	$(CC1) -c p2pregistry.c

UDP_Client: $C
	$(CC1) -o UDP_Client $C

UDP_Client.o: UDP_Client.c
	$(CC1) -c UDP_Client.c

clean:
	rm *.o
	rm p2pregistry
	rm TCP_Client
	rm UDP_Client        
