// Client side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <arpa/inet.h>
#define PORT 8080 
#define BUFFER_SIZE 1024


using namespace std;

//save socket #
struct thread_data{
	int sock;
};

//thread function for reading in input constantly
void* outcomingMessagePrompter(void *t){
	struct thread_data *td;
	td = (struct thread_data *) t;
	while(1){
		char outBuffer[BUFFER_SIZE]="\0"; 
    	fgets(outBuffer, BUFFER_SIZE, stdin);
		printf("\n"); 
		send(td->sock , outBuffer , strlen(outBuffer) , 0);
	}

	return NULL;
}

//thread function for reading in messages coming from server constantly
void* incomingMessagePrinter(void *t){
	int valread;
	struct thread_data *td;
	td = (struct thread_data *) t;
	while(1){
		char inBuffer[BUFFER_SIZE]="\0"; 
		valread = read( td->sock , inBuffer, BUFFER_SIZE); 
		if(strlen(inBuffer)>0){
			printf("\n\nServer says: %s\n\n", inBuffer );
		}
	}

	return NULL;
}

int main(int argc, char const *argv[]) 
{ 
	struct sockaddr_in address; 
	int sock = 0, valread; 
	struct sockaddr_in serv_addr;  
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}


	//make two threads. one for printing incoming messages, one for reading in outcoming messages.
	pthread_t incomingMessageThread;
	pthread_t outcomingMessageThread;

	//make thread data
	struct thread_data td;
	td.sock = sock;

	//start threads
	int rc1 = pthread_create(&incomingMessageThread, NULL, incomingMessagePrinter, (void *)&td);
	int rc2 = pthread_create(&outcomingMessageThread, NULL, outcomingMessagePrompter, (void *)&td);

	//exit if threads could not be created
	if (rc1 || rc2) {
        printf("Error:unable to create thread\n");
        exit(-1);
    }

	//end threads when they are done.
	pthread_join(incomingMessageThread, NULL);
	pthread_join(outcomingMessageThread, NULL);

	pthread_exit(NULL);

	return 0; 
}