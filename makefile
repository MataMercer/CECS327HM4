CC1=g++ -O3 -Wall -pthread -std=c++14

CFLAGS = -g -c -Wall -pthread -std=c++14
#CFLAGS = -ansi -c -Wall -pedantic
all: TCP_Client p2pregistry UDP_Client Client

A = TCP_Client.o
B = p2pregistry.o
C = UDP_Client.o
D = Client.o

TCP_Client: $A
	$(CC1) -o TCP_Client $A

TCP_Client.o: TCP_Client.cpp
	$(CC1) -c TCP_Client.cpp

p2pregistry: $B
	$(CC1) -o p2pregistry $B

p2pregistry.o: p2pregistry.cpp
	$(CC1) -c p2pregistry.cpp

UDP_Client: $C
	$(CC1) -o UDP_Client $C

UDP_Client.o: UDP_Client.cpp
	$(CC1) -c UDP_Client.cpp


Client: $D
	$(CC1) -o Client $D

Client.o: Client.cpp
	$(CC1) -c Client.cpp

clean:
	rm *.o
	rm p2pregistry
	rm TCP_Client
	rm UDP_Client
	rm Client        

