// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#define PORT_TCP 8080 
#define PORT_UDP 5001
#define BUFFER_SIZE 1024
#define MAXLINE 1024 
#define CLIENT_KICK_TIME 10


using namespace std;

//declare mutex variable as global.
// pthread_mutex_t mutex1;

//pack arguments into a struct so you can send it to the thread function as seen below. 
struct thread_data{
	int server_socket;
	unordered_map<int, int>* registeredClients;//clientID to seconds elapsed since 1970
	unordered_map<string, int>* files;//file name to clientID (owner of the file)

	//if using udp use these fields
	sockaddr_in udp_server_socket;
	int udpfd;
	fd_set rset; 
	int maxfdp1;
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



void registerFiles(string inputString, unordered_map<string, int>* files, int ID){
	
	stringstream ss(inputString);
	ss << inputString;
	while( ss.good() )
	{
		string substr;
		getline( ss, substr, ',' );
		(*(files)).insert({substr, ID});
	}
	
}

void separateStringIntoTokens(string inputString, vector<string>* tokens){
    string buf;                 // Have a buffer string
    stringstream ss(inputString);       // Insert the string into a stream

    while (ss >> buf)
        tokens->push_back(buf);
}


//thread function for reading in messages coming from server constantly
void* clientRegisterListener(void *t){
	cout << "[Client Register Listener]: Client register listener started!" << endl;
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

			cout << "[Client Register Listener]: Received message: " << inBuffer << endl; 
			
			//convert buffer into C++ string, tokenize the string by separating by space, put it in a vector
			vector<string>* tokens = new vector<string>();
			string bufferString = inBuffer;
			separateStringIntoTokens(bufferString, tokens);

			//find the command from the vector and client ID. 
			string command = (*tokens).at(0);

			if(command == "register"){
				//generate ID and put it on the outbuffer
				//critical section start
				//  pthread_mutex_lock(&mutex1); 
				int ID = generateUniqueID(td->registeredClients);
				td->registeredClients->insert({ID, time(0)});
				// pthread_mutex_unlock(&mutex1);
				//critical section end
				char outBuffer[BUFFER_SIZE];
				sprintf(outBuffer, "%s%d", "registeredID " ,ID);

				//send ID to client via the outbuffer
				send(client_socket, outBuffer , strlen(outBuffer) , 0);

				//TODO: add the files the client has to the files list/hashmap.
				(*(td->registeredClients))[ID] = time(0);
				string filesString = (*tokens).at(1);
				registerFiles(filesString, td->files ,ID);
				cout << "[Client Register Listener]: Client with ID " << ID << " and its files have been registered." << endl; 
			}
			else if(command == "filesearch"){
				string requestedFile = (*tokens).at(1);
				int fileOwnerID = -1;
				if ((td->files)->find(requestedFile) != (td->files)->end()){
					fileOwnerID = (*(td->files))[requestedFile];
					cout << "[Client Register Listener]: The owner of file " << requestedFile << " is " << fileOwnerID << endl;
				}else{
					cout << "[Client Register Listener]: Unable to find file " << requestedFile << endl;
				}
				

				char outBuffer[BUFFER_SIZE];
				sprintf(outBuffer, "%s%d", "filesearchresult ", fileOwnerID);
				//send ID to client via the outbuffer
				send(client_socket, outBuffer , strlen(outBuffer) , 0);
			}
			else{
				cout << "![Client Register Listener]: Client register listener received a message that was neither 'register <file1, file2, filen>' or 'filesearch file1'"<< endl;
			}
			//close connection with client
			close(client_socket);
		}
		
	}

	return NULL;
}






//TODO:
//Listen for client UDP pings to the server showing its alive. 
void* clientPingListener(void *t){
	cout << "[Client Ping Listener]: Client ping listener started!" <<endl;
	struct thread_data *td;
	td = (struct thread_data*) t;
	ssize_t n; 
	socklen_t len; 
	struct sockaddr_in cliaddr; 
	
	memset(&cliaddr, 0, sizeof(cliaddr)); 
	len = sizeof(cliaddr); 
	while(1){
		
		FD_SET(td->udpfd, &(td->rset)); 
		// select the ready descriptor 
		int nready = select(td->maxfdp1, &(td->rset), NULL, NULL, NULL); 
		if (FD_ISSET(td->udpfd, &(td->rset))) { 

			//init buffer to place the received messages into. 
			char buffer[BUFFER_SIZE];
			bzero(buffer, sizeof(buffer)); 

			// if udp socket is readable receive the message. 
			n = recvfrom(td->udpfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &cliaddr, &len); 
			buffer[n] = '\0'; 
			
			if(n>0){
				cout << "\n[Client Ping Listener]: Received message: " << buffer <<endl;

				//convert buffer into C++ string, tokenize the string by separating by space, put it in a vector
				vector<string>* tokens = new vector<string>();
				string bufferString = buffer;
				separateStringIntoTokens(bufferString, tokens);

				//find the command from the vector and client ID. 
				string command = (*tokens).at(0);
				int clientID = stoi((*tokens).at(1));

				//if the command is Ping (it should be the only command), update the client's last ping timestamp in registeredClients. Else report incorrect formatted message. 
				if(command == "ping"){
					if ((td->registeredClients)->find(clientID) != (td->registeredClients)->end()){
						cout << "[Client Ping Listener]: Updated the timestamp for client ID " << clientID << endl;  
						(*(td->registeredClients))[clientID] = time(0);
					}
					else{
						cout << "![Client Ping Listener]: The ping came from a client with an ID that is not registered." << endl;
					}
				}else{
					cout << "![Client Ping Listener]: Client ping listener received a UDP message that was not in the format 'ping <clientID>'." << endl;
				}
				
				delete tokens;
			}
		}else{
			cout << "FD NOT SET" << endl;
		}


	}

	return NULL;
}


//TODO:
//constantly cycle through the registered clients and check if the last ping time is past the expiration time. 
void* clientStatusChecker(void *t){
	cout << "[Client Status Checker] Client status checker has started!" << endl;
	struct thread_data *td;
	td = (struct thread_data*) t;

	while(1){

		for(auto& item: *(td->registeredClients) ){
			
			//if client's last ping time exceeds the constant we specified, remove from registered clients.
			int timeDiff = time(0) - item.second;  
			
			if(timeDiff > CLIENT_KICK_TIME){
				int id = item.first;
				td->registeredClients->erase(id);

				for(auto& fileItem: *(td->files)){
					string filename = fileItem.first;
					int fileOwnerID = fileItem.second;

					if(fileOwnerID == id){
						td->files->erase(filename);
						
					}
				}
				cout << "[Client Status Checker] Client ID " << id << " timed out. It and all its files have been removed." << endl;
			}
			
		}
		sleep(1);
	}

	return NULL;
}




int main(int argc, char const *argv[]) 
{ 
	
	//TCP
	//Create a socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//Define the server address 
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT_TCP);
	server_address.sin_addr.s_addr = INADDR_ANY;

	//Bind the socket to the IP and port
	bind(server_socket, (struct sockaddr *) &server_address, 
	sizeof(server_address));

	//Listen for connections
	listen(server_socket, 5);





	
	//UDP 
	/* create UDP socket */
	int udpfd;
	if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("cannot create socket\n");
                return 0;
        }
	fd_set rset; 
	// binding server addr structure to udp sockfd 
	struct sockaddr_in UDP_server_address;
	UDP_server_address.sin_family = AF_INET;
	UDP_server_address.sin_port = htons(PORT_UDP);
	UDP_server_address.sin_addr.s_addr = INADDR_ANY;
	if(bind(udpfd, (struct sockaddr*)&UDP_server_address, sizeof(UDP_server_address))<0)    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
	// clear the descriptor set 
	FD_ZERO(&rset); 

	// get maxfd 
	int maxfdp1 = max(server_socket, udpfd) + 1; 









	//init the data structures to store client info
	unordered_map<int, int>* registeredClients = new unordered_map<int, int>();//store a map of clientID:timeOfLastPing. note: time of last ping is just seconds since 1970/epoch. 
	unordered_map<string, int>* files = new unordered_map<string, int>();//store a dict/hashmap of filename:clientID to make finding the file owner quick and easy. 

	//init mutex
    // pthread_mutex_init(&mutex1, 0); //init mutex1


	//make thread data
	struct thread_data td1;
	td1.server_socket = server_socket;
	td1.registeredClients = registeredClients;
	td1.files = files;

	td1.udp_server_socket = UDP_server_address;
	td1.udpfd = udpfd;
	td1.rset = rset; 
	td1.maxfdp1 = maxfdp1;



	//make two threads. one for printing incoming messages, one for reading in outcoming messages.
	pthread_t clientRegisterListenerThread;
	pthread_t clientPingListenerThread;
	pthread_t clientStatusListenerThread;


	cout << "Starting threads..." << endl;
	//start threads
	int rc1 = pthread_create(&clientRegisterListenerThread, NULL, clientRegisterListener, (void *)&td1);
	int rc2 = pthread_create(&clientPingListenerThread, NULL, clientPingListener, (void *)&td1);
	int rc3= pthread_create(&clientStatusListenerThread, NULL, clientStatusChecker, (void *)&td1);

	//end if threads could not start
	if (rc1 || rc2) {
        printf("Error: unable to create threads\n");
        exit(-1);
    }

	//end thrads
	pthread_join(clientRegisterListenerThread, NULL);
	pthread_join(clientPingListenerThread, NULL);
	pthread_join(clientStatusListenerThread, NULL);

	pthread_exit(NULL);


	//clean up heap
	delete registeredClients;
	delete files; 

	//clean up mutex
	// pthread_mutex_destroy(&mutex1); 
	return 0; 
} 

