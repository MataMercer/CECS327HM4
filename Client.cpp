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
//set up get and set ids for the broadcast listener to use
string id; 

void setClientID(int i)
{
    id=to_string(i);
}
string getClientID()
{
    return id;
}

//broadcasts the clientID that the client requester wants to find and filename that the client requester wants
void broadcaster (int id,string fileN) {

	int sock;
	string broadcastIPs="127.0.0.1";
	const char *broadcastIP=broadcastIPs.c_str();
	int broadcastPort=PORT;
	string clientID = to_string(id);
    string fileName=fileN;
	
	string sendStringS="broad " + clientID +" " + fileName; //what the message is consisted of. broad to know it's for a broadcasting message
	const char* sendString=sendStringS.c_str();

	unsigned int sendStringLen=sendStringS.length();

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
       printf("socket() failed");
    }
	int broadcastPermission;
	struct sockaddr_in broadcastAddr; 

    /* Set socket to allow broadcast */
    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0)
    {
       printf("setsockeopt() failed");
       exit(0);
    }

    /* Construct local address structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);/* Broadcast IP address */
    broadcastAddr.sin_port = htons(broadcastPort);         /* Broadcast port */


    sendto(sock, sendString, sendStringLen, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
}

//thread that listens to the broadcast to see if the client themselves need to be messaged about a file request
void* broadcastListener(void *t)
{
	struct thread_data *td;
	td = (struct thread_data*) t;                      /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned short broadcastPort=PORT;     /* Port */
    char recvString[256]; /* Buffer for received string */
    int recvStringLen;                /* Length of received string */

    /* Construct bind structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */

    broadcastAddr.sin_port = htons(broadcastPort);      /* Broadcast port */

    while(1) {
	if ((td->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Socket() failed\n");
	}

	/* Bind to the broadcast port */
	if (bind(td->sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
	{
		//printf("bind() failed\n");
		close(td->sock);
        continue;
	}

	/* Receive a single datagram from the server */
	if ((recvStringLen = recvfrom(td->sock, recvString, 256, 0, NULL, 0)) < 0)
	{
		printf("recvfrom() failed");
	}
	
	recvString[recvStringLen] = '\0';

    //breaks down the message and splits them to their respective names for comparison
    vector<string>* tokens = new vector<string>();
    string recStr(recvString);
    separateStringIntoTokens(recStr, tokens);
    if((tokens->at(0))=="broad")
    {
        if(tokens->at(1)==getClientID())
        {
            cout<<"Request for " + tokens->at(2)<<endl;
        }
    }

    delete tokens;
	close(td->sock);
    }
    exit(0);
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
                    broadcaster(ownerID,filename);

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
	int sock = 0; 
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

    pthread_t broadcastThread;

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
            setClientID(*(clientID)); //sets client id for broadcast listener to use
            //start pinging once registered
            int rc2 = pthread_create(&pingerThread, NULL, pinger, (void *)&td);
            int rc3 = pthread_create(&broadcastThread, NULL, broadcastListener, (void *)&td);
             if (rc2||rc3) {
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
			exit(1);
		}
	}



	//end threads when they are done.
    pthread_join(pingerThread, NULL);
    pthread_join(broadcastThread,NULL);

	pthread_exit(NULL);

	return 0; 
}