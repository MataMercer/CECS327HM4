// Client side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#define PORT 8080 
#define UDP_PORT 5001
#define BUFFER_SIZE 1024
#define PING_TIME 5


using namespace std;

//save socket # and client ID
struct thread_data{
	int sock;
    int* clientID;
};

//take string and separate it into words and put words into a vector. aka tokenization.
void separateStringIntoTokens(string inputString, vector<string>* tokens){
    string buf;                 // Have a buffer string
    stringstream ss(inputString);       // Insert the string into a stream

    while (ss >> buf)
        tokens->push_back(buf);
}


//constantly ping the server every x seconds to let the server know this client is alive. 
void* pinger(void *t){
    struct thread_data *td;
	td = (struct thread_data *) t;
    cout << "Pinger thread started. Pinging with ID " << *(td->clientID) << endl; 
    while(1){
        //ping every x seconds. 
        sleep(PING_TIME);

        //send the message 'ping <clientID>' 
        int sockfd; 
        char buffer[BUFFER_SIZE]; 
        sprintf(buffer, "ping %d", *(td->clientID));
        struct sockaddr_in servaddr; 

        int n, len; 
        // Creating socket file descriptor 
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
            printf("socket creation failed"); 
            exit(0); 
        } 

        memset(&servaddr, 0, sizeof(servaddr)); 

        // Filling server information 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_port = htons(UDP_PORT); 
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 


        // send hello message to server 
        sendto(sockfd, buffer, strlen(buffer), 
            0, (const struct sockaddr*)&servaddr, 
            sizeof(servaddr)); 

        close(sockfd); 
    }
    return NULL;
}

//register the client by sending a message 'register <file1,file2,filen>'. The server will interpret this as a registration request and return the message
//'registeredID <AssignedID>' to which the client will use as its ID. 
int registerClientToServer(int socket, string filenames){ 

        int assignedID = -1; 
        char outBuffer[BUFFER_SIZE]="\0"; 
        
    	sprintf(outBuffer, "%s%s", "register ", filenames.c_str());
		printf("\n"); 
		send(socket , outBuffer , strlen(outBuffer) , 0);

        char inBuffer[BUFFER_SIZE]="\0"; 
		int valread = read( socket , inBuffer, BUFFER_SIZE); 
		if(strlen(inBuffer)>0){
			cout << "Server message received: " << inBuffer << endl;
            string bufferString = inBuffer;
            vector<string>* tokens = new vector<string>();
            separateStringIntoTokens(bufferString, tokens);
            string command = tokens->at(0);
            if(command=="registeredID"){
                assignedID = stoi(tokens->at(1));
                cout << "Client now has ID of " << assignedID << endl;
            }else{
                cout << "Unknown command received from server.";
            }


            delete tokens;
		}
    close(socket); 
    return assignedID; 
}

//send a filename to search up to the server and the server will respond with what client has that filename. It does this by sending 
//a message 'filesearch <filename>' and the server responds with 'filesearchresult <ownerIDofFilename>'. If ownerID is -1, it is not found.
//Next, the client broadcasts this id and the client with that id will print out a message achknowledging it. 
void fileSearch(int socket, string filename){
        cout << "Searching for file..." << endl;
        char outBuffer[BUFFER_SIZE]="\0"; 
        
    	sprintf(outBuffer, "%s%s", "filesearch ", filename.c_str());
		printf("\n"); 
		send(socket , outBuffer , strlen(outBuffer) , 0);

        char inBuffer[BUFFER_SIZE]="\0"; 
		int valread = read( socket , inBuffer, BUFFER_SIZE); 
		if(strlen(inBuffer)>0){
			cout << "Server message received: " << inBuffer << endl;
            string bufferString = inBuffer;
            vector<string>* tokens = new vector<string>();
            separateStringIntoTokens(bufferString, tokens);
            string command = tokens->at(0);
            if(command=="filesearchresult"){
                int ownerID = stoi(tokens->at(1));
                if(ownerID != -1){
                    cout << "File is located in Client ID " << ownerID << endl;

                    //TODO: BROADCAST FILENAME AND THIS CLIENTID.

                }else{
                    cout << "Unable to find the file." << endl;
                }
            }else{
                cout << "Unknown command received from server.";
            }


            delete tokens;
		}
        close(socket); 
    
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


    //put client ID at a location on the HEAP so we can read/write it directly from any function 
    int *clientID = new int();
    //if unassigned, clientID will be -1.
    *(clientID) = -1;
    
    bool isPublished=false;
    string files;

    //make thread data
	struct thread_data td;
	td.sock = sock;
    td.clientID = clientID;

    //init thread vars
    pthread_t pingerThread;

    while(1)
	{
		int options=0;
		
		
		cout<<"Menu: \n1. REGISTER \n2. LOAD FILES TO CLIENT \n3. SEARCH \n4. EXIT"<<endl;
		cin>>options;
		if (options==1&&isPublished)
		{
                //Reconnect to the server 
            	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                { 
                    printf("\n Socket creation error \n"); 
                    return -1; 
                } 
                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
                { 
                    printf("\nConnection Failed \n"); 
                    return -1; 
                }
                
			cout<<"Registering..."<<endl;
            *clientID = registerClientToServer(sock, files);

            //start pinging once registered
            int rc2 = pthread_create(&pingerThread, NULL, pinger, (void *)&td);
             if (rc2) {
                printf("Error:unable to create thread\n");
                exit(-1);
            }
			
		}
		else if (options==1&&!isPublished)
		{
			cout<<"Please PUBLISH first"<<endl;

		}
		else if (options==2)
		{
			cout<<"PUBLISH"<<endl;
			cout<<"Enter the file names you want to publish separated by commas with NO SPACES. i.e. file1,file2,file3 (They do not need to exist): "<<endl;
			cin>>files;
            cout << "Loaded Files: " << files << endl;  
            // files = "file1,file2";
			isPublished=true;

		}
		else if (options==3)
		{
                //reconnect to the server
            	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
                { 
                    printf("\n Socket creation error \n"); 
                    return -1; 
                } 

                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
                { 
                    printf("\nConnection Failed \n"); 
                    return -1; 
                }

			cout<<"SEARCH"<<endl;
            string filename;
            cin >> filename; 
            fileSearch(sock, filename);

		}
		else
		{
			cout<<"Exiting"<<endl;
			break;
		}
	}



	//end threads when they are done.
    pthread_join(pingerThread, NULL);

	pthread_exit(NULL);

	return 0; 
}