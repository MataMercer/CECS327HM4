// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <unordered_map>
#include <string>
#include <iostream>
#define PORT 8080 
#define PORT_UDP 8081
#define BUFFER_SIZE 1024
#define MAXLINE 1024 
#define CLIENT_KICK_TIME 5

using namespace std;


//pack arguments into a struct so you can send it to the thread function as seen below. 
struct thread_data{
	int server_socket;
	unordered_map<int, int>* registeredClients;//clientID to seconds elapsed since 1970
	unordered_map<string, int>* files;//file name to clientID (owner of the file)
};




//generate random number to use as the ID for the client. We make sure it's unique by checking the registeredClients hashmap for duplicates.
int generateUniqueID(unordered_map<int, int>* registeredClients){
    srand(time(0)); 
    int upper = registeredClients->size()+1;
    int lower = 0;
    int alreadyExistsFlag = 1;
    int newID = 0;
    while(alreadyExistsFlag==1){

        newID = (rand() % (upper - lower + 1)) + lower; 
		alreadyExistsFlag = registeredClients->find(newID) != registeredClients->end();
    }
    return newID;
}


//thread function for reading in messages coming from server constantly
void* clientRegisterListener(void *t){
	struct thread_data *td;
	td = (struct thread_data *) t;
	while(1){
		//connect to client and fetch its socket
		int client_socket = accept(td->server_socket, NULL, NULL);

		//accept client message
		char inBuffer[BUFFER_SIZE] = "\0"; 
		read( client_socket, inBuffer, BUFFER_SIZE); 
		
		//if something was sent, register
		if(strlen(inBuffer)>0){

			printf("\n\nClient says: %s\n\n", inBuffer );
			
			//generate ID and put it on the outbuffer
			int ID = generateUniqueID(td->registeredClients);
			td->registeredClients->insert({ID, time(0)});
			char outBuffer[BUFFER_SIZE];
			sprintf(outBuffer, "%d", ID);

			//TODO: add the files the client has to the files list/hashmap. 

			//send ID to client via the outbuffer
			send(client_socket, outBuffer , strlen(outBuffer) , 0);

			//close connection with client
			close(client_socket);
		}
		
	}

	return NULL;
}


//TODO:
//Listen for client UDP pings to the server showing its alive. 
void* clientPingListener(void *t){
	struct thread_data *td;
	td = (struct thread_data*) t;

	while(1){

	}

	return NULL;
}


//TODO:
//constantly cycle through the registered clients and check if the last ping time is past the expiration time. 
void* clientStatusChecker(void *t){
	struct thread_data *td;
	td = (struct thread_data*) t;

	while(1){
		for(auto& item: *(td->registeredClients) ){
			//if client's last ping time exceeds the constant we specified, remove from registered clients. 
			if(time(0) - item.second > CLIENT_KICK_TIME){
				td->registeredClients->erase(item.first);

				//TODO erase all files under 
			}
		}
	}

	return NULL;
}

//TODO:
//constantly listen for requests to file searches. Locates the client that has the file and sends that location to the requester so they can connect client-to-client. 
void* fileSearchListener(void *t){
	struct thread_data *td;
	td = (struct thread_data*) t;

	while(1){

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


	//init the data structures to store client info
	unordered_map<int, int>* registeredClients = new unordered_map<int, int>();//store a map of clientID:timeOfLastPing. note: time of last ping is just seconds since 1970/epoch. 
	unordered_map<string, int>* files = new unordered_map<string, int>();//store a dict/hashmap of filename:clientID to make finding the file owner quick and easy. 

	//make thread data
	struct thread_data td1;
	td1.server_socket = server_socket;
	td1.registeredClients = registeredClients;




	//make two threads. one for printing incoming messages, one for reading in outcoming messages.
	pthread_t incomingMessageThread;
	// pthread_t outcomingMessageThread;

	//start threads
	int rc1 = pthread_create(&incomingMessageThread, NULL, clientRegisterListener, (void *)&td1);
	// int rc3= pthread_create(&outcomingMessageThread, NULL, clientStatusChecker, (void *)&td2);

	//end if threads could not start
	if (rc1) {
        printf("Error: unable to create threads\n");
        exit(-1);
    }

	//end thrads
	pthread_join(incomingMessageThread, NULL);
	// pthread_join(outcomingMessageThread, NULL);

	pthread_exit(NULL);

	delete registeredClients;
	delete files; 
	return 0; 
} 

