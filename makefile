CC1=g++ -O3 -Wall -pthread -std=c++14

CFLAGS = -g -c -Wall -pthread -std=c++14
#CFLAGS = -ansi -c -Wall -pedantic
all: p2pregistry Client

B = p2pregistry.o
D = Client.o

p2pregistry: $B
	$(CC1) -o p2pregistry $B

p2pregistry.o: p2pregistry.cpp
	$(CC1) -c p2pregistry.cpp

Client: $D
	$(CC1) -o Client $D

Client.o: Client.cpp
	$(CC1) -c Client.cpp

clean:
	rm *.o
	rm p2pregistry
	rm Client        

