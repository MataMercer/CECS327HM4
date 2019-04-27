// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#define PORT 8080 
#define PORT_UDP 8081
#define BUFFER_SIZE 1024
#define MAXLINE 1024 

using namespace std;

struct thread_data{
	int server_socket;
	int *registeredClientIDs;
    int *numberOfRegisteredClients;
};

// //thread function for reading in input constantly
// void outcomingMessagePrompter(void *t){
// 	struct thread_data *td;
// 	td = (struct thread_data *) t;
// 	while(1){
// 		char outBuffer[BUFFER_SIZE] = "\0";
//     	fgets(outBuffer, BUFFER_SIZE, stdin); 
// 		printf("\n");
// 		send(td->new_socket , outBuffer , strlen(outBuffer) , 0);
// 	}
// }

int generateUniqueID(int* registeredClients, int numberOfRegisteredClients){
    srand(time(0)); 
    int upper = BUFFER_SIZE;
    int lower = 0;
    int alreadyExistsFlag = 1;
    int newID = 0;
    while(alreadyExistsFlag==1){
        alreadyExistsFlag = 0;
        newID = (rand() % (upper - lower + 1)) + lower; 
        for(int i=0; i<numberOfRegisteredClients; i++){
            if(registeredClients[i] == newID){
                alreadyExistsFlag = 1;
                break;
            }
        }
    }
    return newID;
}


//thread function for reading in messages coming from server constantly
void* incomingMessagePrinter(void *t){
	struct thread_data *td;
	td = (struct thread_data *) t;
	while(1){
		int client_socket = accept(td->server_socket, NULL, NULL);
		char inBuffer[BUFFER_SIZE] = "\0"; 
		read( client_socket, inBuffer, BUFFER_SIZE); 
		
		if(strlen(inBuffer)>0){

			printf("\n\nClient says: %s\n\n", inBuffer );
			
			//generate ID and put it on the outbuffer
			int ID = generateUniqueID(td->registeredClientIDs, *(td->numberOfRegisteredClients));
			if(*(td->numberOfRegisteredClients) < BUFFER_SIZE){
				td->registeredClientIDs[*(td->numberOfRegisteredClients)] = ID;
				*(td->numberOfRegisteredClients)++;
			}
			char outBuffer[BUFFER_SIZE];
			sprintf(outBuffer, "%d", ID);

			send(client_socket, outBuffer , strlen(outBuffer) , 0);

			//close connection with client
			close(client_socket);
		}
		
	}

	return NULL;
}






int main(int argc, char const *argv[]) 
{ 
	//Create a socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//Define the server address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	//Bind the socket to the IP and port
	bind(server_socket, (struct sockaddr *) &server_address, 
	sizeof(server_address));

	//Listen for connections
	listen(server_socket, 5);


	//init vars
	int* registeredClientIDs = (int*) malloc(BUFFER_SIZE * sizeof(int));
	int* numberOfRegisteredClients = (int*) malloc(sizeof(int));
	numberOfRegisteredClients[0] = 0;

	//make thread data
	struct thread_data td;
	td.server_socket = server_socket;
	td.registeredClientIDs = registeredClientIDs;
	td.numberOfRegisteredClients = numberOfRegisteredClients;

	//make two threads. one for printing incoming messages, one for reading in outcoming messages.
	pthread_t incomingMessageThread;
	// pthread_t outcomingMessageThread;

	//start threads
	int rc1 = pthread_create(&incomingMessageThread, NULL, incomingMessagePrinter, (void *)&td);
	// int rc2 = pthread_create(&outcomingMessageThread, NULL, outcomingMessagePrompter, (void *)&td);

	//end if threads could not start
	if (rc1) {
        printf("Error: unable to create thread\n");
        exit(-1);
    }

	//end thrads
	pthread_join(incomingMessageThread, NULL);
	// pthread_join(outcomingMessageThread, NULL);

	pthread_exit(NULL);

	free(registeredClientIDs);
	return 0; 
} 

