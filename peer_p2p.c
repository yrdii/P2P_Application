#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/time.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>                                                                         
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define	BUFLEN 3000 //Max bytes per packet

struct pdu {
    char type;
    char data[100];
};

struct R_pdu {
    char type;
    char peerName[10];
    char contentName[10];
    char host[5];
    char port[6];
};

struct D_pdu {
    char type;
    char peerName[10];
    char content[90];
};

struct T_pdu { 
    char type;
    char peerName[10];
    char contentName[10];
};

struct C_pdu {
	char type;
    char content[1459];
};

struct A_pdu {
	char type;
    char peerName[10];
};

struct S_pdu {
    char type;
    char peerName[10];
    char contentNameOrAddress[90];
};

struct O_pdu {
	char type;
    char contentList[100]; 
};

struct E_pdu {
	char type;
    char errMsg[100];
};

//Variables
char peerName[10];                          //Peer name
char localContentName[5][10];               //The name of the local content that has been registered; max 5
char localContentPort[5][6];                //The port number associated with each locally registered content
int numOfLocalContent = 0;                  //Stores number of saved local content

//TCP Socket variables
int fdArray[5] = {0};
struct sockaddr_in socketArray[5];
int max_sd = 0;
int activity;                               //Tracks I/O activity on sockets
fd_set  readfds;                            //List of socket descriptors

//UDP connection variables
char	*host = "localhost";                //Default UDP host
int		port = 3000;                        //Default UDP port                
int		s_udp, s_tcp, new_tcp, n, type;	    //Socket descriptor and socket type	
struct 	hostent	*phe;	                    //Pointer to host information entry	


/*-----Add info about local content-----*/
void addToLocalContent(char contentName[], char port[], int socket, struct sockaddr_in server){
    
    //Copy content name and port into corresponding arrays at index numOfLocalContent
    strcpy(localContentName[numOfLocalContent], contentName);
    strcpy(localContentPort[numOfLocalContent], port);

    //Store the socket and server information at index numOfLocalContent
    fdArray[numOfLocalContent] = socket;
    socketArray[numOfLocalContent] = server;

    //Update the maximum socket descriptor if the current socket is greater than it
    if (socket > max_sd){
        max_sd = socket;
    }

    printf("-----------------------------\n");
    fprintf(stderr, "Added the content to the local list of contents\n");

    //Increment the count of local content
    numOfLocalContent++;
}

/*-----Remove Content from Local Content*/
void removeFromLocalContent(char contentName[]){
    int j;
    int foundElement = 0;

    //Look for the requested content name
    for(j = 0; j < numOfLocalContent; j++){
        if(strcmp(localContentName[j], contentName) == 0){

            //Name is found, delete element in both name and port lists
			printf("----------------------------------------------\n");
            printf("The following content was deleted:\n");
            printf("    Content Name: %s\n", localContentName[j]);
            printf("    Port: %s\n\n", localContentPort[j]);
			printf("\n");

            //Moves all elements underneath it up by one
            memset(localContentName[j], '\0', sizeof(localContentName[j]));         
            memset(localContentPort[j], '\0', sizeof(localContentPort[j]));  
            fdArray[j] = 0;       
            while(j < numOfLocalContent - 1){
                strcpy(localContentName[j], localContentName[j+1]);
                strcpy(localContentPort[j], localContentPort[j+1]);
                fdArray[j] = fdArray[j+1];
                socketArray[j] =  socketArray[j+1];
                j++;
            }
            foundElement = 1;
            break;
        }
    }

    //Requested content was not found
    if(!foundElement){
        printf("------------------------------\n");
        printf("Error, the requested content name was not found in the list:\n");
        printf("    Content Name: %s\n\n", contentName);
    }else{
        numOfLocalContent = numOfLocalContent -1;
    }
}

/*-----Displays All Task Options-----*/
void printTasks(){
	printf("----------------------------------------------\n");
    printf("Possible Options:\n");
    printf("    R: Content Registeration\n");
    printf("    T: Content De-register\n");
    printf("    O: List all registered content\n");
    printf("    L: List all local registered content\n");
    printf("    D: Content Download\n");
    printf("    Q: Quit and Content De-register \n");
}

/*-----User Input-----*/
void printTasksShort(){
	printf("----------------------------------------------\n");
    printf("Choose a option:\n");
    printf("    R, T, O, L, D, Q \n");
	printf("    I: info about the options \n");
}

/*-----Register Content-----*/
void registerContent(char contentName[]){
    struct  R_pdu    packetR;                //The PDU packet to send to the index server
    struct  pdu     sendPacket;

    char    writeMsg[101];                  //Temp for outgoing message to index server
    int     readLength;                     //Length of incoming data bytes from index server
    char    readPacket[101];                //Temp for incoming message from index server
    
    //Verify that the file exists locally
    if(access(contentName, F_OK) != 0){
        // File does not exist; print error message
        printf("Error: File %s does not exist locally\n", contentName);
        return;
    }

    //TCP connection variables        
    struct 	sockaddr_in server, client;
    struct  pdu     incomingPacket;
    char    sendContent[sizeof(struct C_pdu)];
    char    tcp_host[5];
    char    tcp_port[6];
    int     alen;
    int     bytesRead;
    int     m;
    int     i;

    //Creates a TCP stream socket and retrieves the host and port info for later use
	if ((s_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR: Can't create a TCP socket\n");
		exit(1);
	}
    //Initialize server structure
    bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(s_tcp, (struct sockaddr *)&server, sizeof(server));//Bind sockete to a local address
    //Get local socket name(host&port)
    alen = sizeof(struct sockaddr_in);
    getsockname(s_tcp, (struct sockaddr *)&server, &alen);
    //Convert to string
    snprintf(tcp_host, sizeof(tcp_host), "%d", server.sin_addr.s_addr);
    snprintf(tcp_port, sizeof(tcp_port), "%d", htons(server.sin_port));

    //Build R type PDU to send to index server
    memset(&packetR, '\0', sizeof(packetR));
    memcpy(packetR.contentName, contentName, 10);
    memcpy(packetR.peerName, peerName, 10);
    memcpy(packetR.host, tcp_host, sizeof(tcp_host));
    memcpy(packetR.port, tcp_port, sizeof(tcp_port));
    packetR.type = 'R';

    //Switch the R_pdu type into default pdu type for transmission
    //sendPacket.data = [peerName]+[contentName]+[host]+[port]
    memset(&sendPacket, '\0', sizeof(sendPacket));
    int dataOffset = 0;
    sendPacket.type = packetR.type;
    memcpy(sendPacket.data + dataOffset, 
            packetR.peerName, 
            sizeof(packetR.peerName));
    dataOffset += sizeof(packetR.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetR.contentName,
            sizeof(packetR.contentName));
    dataOffset += sizeof(packetR.contentName);
    memcpy(sendPacket.data + dataOffset, 
            packetR.host,
            sizeof(packetR.host));
    dataOffset += sizeof(packetR.host);
    memcpy(sendPacket.data + dataOffset, 
            packetR.port,
            sizeof(packetR.port));

    //Sends PDU to the index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));     
    
    //Wait for message and check the PDU type (A or E)
    readLength = read(s_udp, readPacket, BUFLEN);

    i = 1;
    struct E_pdu packetE;
    struct A_pdu packetA;
    switch(readPacket[0]){       
        case 'E':
            //Error; extract error message
            packetE.type = readPacket[0];
            while(readPacket[i] != '\0'){ 
                packetE.errMsg[i-1] = readPacket[i];
                i++;
            }

            //Output to user; print error message
			printf("----------------------------------------------\n");
            printf("Error registering content:\n");
            printf("    %s\n", packetE.errMsg);
            printf("\n");
            break;
        case 'A':
            //Acknowledgement; extracts info
            packetA.type = readPacket[0];
            while(readPacket[i] != '\0'){ 
                packetA.peerName[i-1] = readPacket[i];
                i++;
            }

            //Output to user; prints details
			printf("----------------------------------------------\n");
            printf("The following content has been successfully registered:\n");
            printf("    Peer Name: %s\n", packetA.peerName);
            printf("    Content Name: %s\n", packetR.contentName);
            printf("    Host: %s\n", packetR.host);
            printf("    Port: %s\n", packetR.port);
            printf("\n");

            //Set up a listening socket
            listen(s_tcp, 5);
            //Handle incoming connections
            switch(fork()){
                case 0: //Child; handle content upload
                    while(1){//Continuously listen for incoming TCP connections
                        int client_len = sizeof(client);
                        char fileName[10];
                        new_tcp = accept(s_tcp, (struct sockaddr *)&client, &client_len);
                        memset(&incomingPacket, '\0', sizeof(incomingPacket));
                        memset(&fileName, '\0', sizeof(fileName));
                        bytesRead = read(new_tcp, &incomingPacket, sizeof(incomingPacket));
                        switch(incomingPacket.type){//Check incoming packet type
                            case 'D'://Download; handle data transfer
				                fprintf(stderr, "-----------------Content Server---------------\n");
                                fprintf(stderr, "Received: D-type packet\n");
                                memcpy(fileName, incomingPacket.data+10, strlen(incomingPacket.data+10));
                                FILE *file;
                                file = fopen(fileName, "rb");//Attempt to open file locally for reading
                                if(!file){//File doesn't exist; print error message
                                    printf("\n");
                                    fprintf(stderr, "File %s does not exist\n", fileName);
                                }else{//File exists; send file to client
									printf("----------------------------------------------\n");
                                    fprintf(stderr, "The content server found file locally and is sending to client\n");
                                    memset(sendContent, '\0', sizeof(sendContent));
                                    sendContent[0] = 'C';
                                    while(fgets(sendContent+1, sizeof(sendContent)-1, file) > 0){
                                        write(new_tcp, sendContent, sizeof(sendContent));
                                        fprintf(stderr, "Wrote the following to client:\n %s\n", sendContent);
                                        //memset(sendContent.content, '\0', sizeof(sendContent.content));
                                    }
                                }
								printf("----------------------------------------------\n");
                                fprintf(stderr, "Closing content server\n");
                                fclose(file);
                                close(new_tcp);
                                break;
                            default://Packet type not D; print error message
                                fprintf(stderr, "ERROR: Content server received an unknown packet\n");
                                break;
                        }
                    }
                    fprintf(stderr, "ERROR: Content server left the loop\n");
                    break;
                case -1: //Parent; fork fails; print error message
                    fprintf(stderr, "Error Creating a child process");
                    break;
            }

            //Add info about local content
            addToLocalContent(packetR.contentName, packetR.port, s_tcp, server);
            break;
        default://Issue reading from server; print error message
            printf("Unable to read incoming message from server\n\n");
    }
}

void deregisterContent(char contentName[], int q){
    struct T_pdu packetT;
    struct pdu sendPacket;
    struct pdu readPacket;

    //Build the T type PDU
    memset(&packetT, '\0', sizeof(packetT));
    packetT.type = 'T';
    memcpy(packetT.peerName, peerName, sizeof(packetT.peerName));
    memcpy(packetT.contentName, contentName, sizeof(packetT.contentName));

    //Switch the T type into a general PDU for transmission
    //sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));
    int dataOffset = 0;
    sendPacket.type = packetT.type;
    memcpy(sendPacket.data + dataOffset, 
            packetT.peerName, 
            sizeof(packetT.peerName));
    dataOffset += sizeof(packetT.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetT.contentName,
            sizeof(packetT.contentName));

    //Sends the data packet to the index server; print indication message
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
	printf("----------------------------------------------\n");
    printf("De-registering Content\n");
	printf("\n");
	// Removes the content from the list of locally registered content
        if (!q) {
        // Find the content index
        int contentIndex = -1;
        int i;
	for (i = 0; i < numOfLocalContent; i++) {
            if (strcmp(localContentName[i], contentName) == 0) {
                contentIndex = i;
                break;
            }
        }

        if (contentIndex != -1) {
            // Delete the file associated with the content
            if (remove(contentName) != 0) {
                perror("Error deleting the file");
            } else {
                printf("File %s deleted successfully.\n", contentName);
            }

            removeFromLocalContent(contentName);
        } else {
            printf("Error, the requested content name was not found in the list:\n");
            printf("    Content Name: %s\n\n", contentName);
        }
    }
    read(s_udp, &readPacket, sizeof(readPacket));
}

/*-----List Locally Registered Content-----*/
void listLocalContent(){
    int j;
    printf("----------------------------------------------\n");
    printf("List of names of the locally registered content:\n");
    printf("Number\t\tName\t\tPort\t\tSocket\n");
    for(j = 0; j < numOfLocalContent; j++){//Iterate through info about locally registered content
        printf("%d\t\t%s\t\t%s\t\t%d\n", j, localContentName[j], localContentPort[j], fdArray[j]);
    }
    printf("\n");
}

/*-----List Registered Content on Index Server*/
void listIndexServerContent(){
    struct O_pdu sendPacket;
    char         readPacket[sizeof(struct O_pdu)];
    
    memset(&sendPacket, '\0', sizeof(sendPacket));
    sendPacket.type = 'O';

    //Send O type PDU to index server
    write(s_udp, &sendPacket, sizeof(sendPacket));

    //Read and list contents
    printf("----------------------------------------------\n");
    printf("The Index Server contains the following:\n");

    //Read response from index server
    int i;
    read(s_udp, readPacket, sizeof(readPacket));
    for(i = 1; i < sizeof(readPacket); i+=10){
        if(readPacket[i] == '\0'){//Reach end of data
            break;
        }else{
            printf("    %d: %.10s\n", i/10, readPacket+i);
        }
    }
    printf("\n");
}

/*-----Request for a Specific Content(?)-----*/
void pingIndexFor(char contentName[]){
    struct S_pdu packetS;
    struct pdu sendPacket;

    //Build the S type packet to send to the index server
    memset(&packetS, '\0', sizeof(packetS));
    packetS.type = 'S';
    memcpy(packetS.peerName, peerName, sizeof(packetS.peerName));
    memcpy(packetS.contentNameOrAddress, contentName, strlen(contentName));

    //Switch the S type into a general PDU for transmission
    //sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));
    int dataOffset = 0;
    sendPacket.type = packetS.type;
    memcpy(sendPacket.data + dataOffset, 
            packetS.peerName, 
            sizeof(packetS.peerName));
    dataOffset += sizeof(packetS.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetS.contentNameOrAddress,
            sizeof(packetS.contentNameOrAddress));

    //Send S type PDU to index server
    write(s_udp, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));
}

/*-----Download Content-----*/
void downloadContent(char contentName[], char address[]){  
    struct 	sockaddr_in server;
    struct	hostent		*hp;
    char    *serverHost = "0";          //The address of the content server
    char    serverPortStr[6];           //Temp to conver string to int        
    int     serverPort;
    int     downloadSocket;
    int     m;

    //Extract port number
    for(m = 5; m<10; m++){
        serverPortStr[m-5] = address[m];
    }
    serverPort = atoi(serverPortStr);
	printf("----------------------------------------------\n");
    printf("Trying to download content from the following address:\n");
    printf("   Host: %s\n", serverHost);
    printf("   Port: %d\n", serverPort);
    
    //Create a TCP stream socket	
	if ((downloadSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR: Can't create a TCP socket\n");
		exit(1);
	}
    bzero((char *)&server, sizeof(struct sockaddr_in));//Clear memory; ready to store info about server's address
	server.sin_family = AF_INET;//Set address family
	server.sin_port = htons(serverPort);//Set server port
	//Attempt to retrive the content server's info
	if (hp = gethostbyname(serverHost))
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if (inet_aton(serverHost, (struct in_addr *) &server.sin_addr) ){
        fprintf(stderr, "ERROR: Can't get server's address\n");
	}

	//Connect to the server 
	if (connect(downloadSocket, (struct sockaddr *)&server, sizeof(server)) == -1){
        fprintf(stderr, "ERROR: Can't connect to server: %s\n", hp->h_name);
        return;
	}
	printf("----------------------------------------------\n");
    fprintf(stderr, "Successfully connected to server at address: %s:%d\n", serverHost, serverPort);
	fprintf(stderr, "\n");

    struct  D_pdu    packetD;
    struct  pdu     sendPacket;

    //Build a D type PDU to send to the content server
    memset(&packetD, '\0', sizeof(packetD));
    packetD.type = 'D';
    memcpy(packetD.peerName, peerName, 10);
    memcpy(packetD.content, contentName, 90);

    //Switch the D-PDU type into a general PDU for transmission
    //sendPacket.data = [peerName]+[contentName]
    memset(&sendPacket, '\0', sizeof(sendPacket));
    int dataOffset = 0;
    sendPacket.type = packetD.type;
    memcpy(sendPacket.data + dataOffset, 
            packetD.peerName, 
            sizeof(packetD.peerName));
    dataOffset += sizeof(packetD.peerName);
    memcpy(sendPacket.data + dataOffset, 
            packetD.content,
            sizeof(packetD.content));

    //Send request to content server
    write(downloadSocket, &sendPacket, sizeof(sendPacket.type)+sizeof(sendPacket.data));

    FILE    *fp;
    struct  C_pdu     readBuffer; 
    int     receivedContent = 0;

    //Download the data
    fp = fopen(contentName, "w+"); //Write the downloaded content
    memset(&readBuffer, '\0', sizeof(readBuffer));
    while ((m = read(downloadSocket, (struct pdu*)&readBuffer, sizeof(readBuffer))) > 0){
        fprintf(stderr, "Received: data from the content server\n");
        fprintf(stderr, "   Type: %c\n", readBuffer.type);
        fprintf(stderr, "   Data: %s\n", readBuffer.content);
        if(readBuffer.type == 'C'){//Writes to local file
            fprintf(stderr, "Received: C-type packet from the content server, starting download\n");
            fprintf(fp, "%s", readBuffer.content);
            receivedContent = 1;
        }else if(readBuffer.type == 'E'){//Error; print error message
            printf("Error parsing C PDU data:\n");
        }else{//Other type; print error message
            fprintf(stderr, "ERROR: Received garbage from the content server, discarding\n");
        }
        memset(&readBuffer, '\0', sizeof(readBuffer));
    }
    fclose(fp);

    //Successfully received content; print message; register content automatically
    if(receivedContent){
        printf("Completed downloading content:\n");
        printf("   Content Name: %s\n", contentName);
        registerContent(contentName);
    }
}

/*-----Read and Log Received Data-----*/
int uploadFile(int sd){
    struct  pdu readPacket;
    int         bytesRead;

    //Read data from socket
    bytesRead = read(sd, (struct pdu*)&readPacket, sizeof(readPacket));
    fprintf(stderr, "Currently handling socket %d\n", sd);
    fprintf(stderr, "   Read in %d bytes of data\n", bytesRead);
    fprintf(stderr, "   Type: %c\n", readPacket.type);
    fprintf(stderr, "   Data: %s\n", readPacket.data);

    return 0;
}

/*-----Main Function-----*/
int main(int argc, char **argv){
    
    //Check input arguments and assign host, port to server
    switch (argc) {
		case 1:
			break;
		case 2:
			host = argv[1];
            break;
		case 3:
			host = argv[1];
			port = atoi(argv[2]);
			break;
		default:
			fprintf(stderr, "usage: UDP Connection to Index Server [host] [port]\n");
			exit(1);
	}

    struct 	sockaddr_in sin;            

    memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                                                                
	sin.sin_port = htons(port);

    //Get host information 
	if (phe = gethostbyname(host)){
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
		fprintf(stderr, "ERROR: Can't get host data \n");                                                               
    
    //Create a UDP; check for errors
	s_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_udp < 0)
		fprintf(stderr, "ERROR: Can't create socket \n");       

    //Connect the UDP to the index server; checks for error
	if (connect(s_udp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "ERROR: Can't connect to %s\n", host);
	}
    
    //Successfully connected; print welcome message
    printf("Hello! You are connected to the Peer-to-Peer Network!\n");
	printf("----------------------------------------------\n");
    
    //Ask for peer name
    printf("Please enter your name: (max 9 characters)\n");
    while(read (0, peerName, sizeof(peerName)) > 10){
        printf("The name is too long!\n");
    }
    
    //Print greeting message with peer name
    peerName[strlen(peerName)-1] = '\0'; 
    printf("Hey %s!\n", peerName);

    char            userChoice[2];          //Store task choice
    char            userInput[10];          //Store additional info required from the user to complete specific tasks
    char            readPacket[101];        //Buffer for incoming messages from the index server            
    int             quit = 0;               //Flag thats enabled when the user wants to quit the app
    int             j;                      //Used for iterative processes
    struct E_pdu    packetE;
    struct S_pdu    packetS;

    while(!quit){
        printTasksShort(); 
        read(0, userChoice, 2);
        memset(userInput, 0, sizeof(userInput));
        // Perform task
        switch(userChoice[0]){
	        case 'I'://User enters I; print task info
		        printTasks(); 
		        break;
            case 'R'://User enters R; ask for content name and register
                printf("Enter the content name you would like to register (max 9 characters):\n");
                scanf("%9s", userInput);      
                registerContent(userInput);
                break;
            case 'T'://User enters T; ask for content name and de-register
                printf("Enter the content name you would like to de-register:\n");
                scanf("%9s", userInput);   
                deregisterContent(userInput,0);
                break;
            case 'D'://User enters D; ask for content name and transfer content
                printf("Enter the content name you would like to download:\n");
                scanf("%9s", userInput);
                pingIndexFor(userInput);
                read(s_udp, readPacket, sizeof(readPacket));
                switch(readPacket[0]){
                    case 'E'://Error
                        j = 1;
                        packetE.type = readPacket[0];
                        while(readPacket[j] != '\0'){//Copy error message
                            packetE.errMsg[j-1] = readPacket[j];
                            j++;
                        }
                        //Print error message
						printf("----------------------------------------------\n");
                        printf("Error downloading content\n");
                        printf("\n");
                        break;
                    case 'S'://Proceed to download
                        packetS.type = readPacket[0];
                        for(j = 0; j < sizeof(readPacket); j++){
                            if (j < 10){//Copy data
                                packetS.peerName[j] = readPacket[j+1]; //1 to 10; readPacket -> peername
                            }
                            packetS.contentNameOrAddress[j] = readPacket[j+11]; //11 to 100; readPacket -> contentname/address
                        }
                        downloadContent(userInput, packetS.contentNameOrAddress);
                        break;
                }
                break;
            case 'O'://User enters O; list all content on index server
                listIndexServerContent();
                break;
            case 'L'://User enters L; list all local content on index server
                listLocalContent();
                break;
            case 'Q'://User enters Q; deregister all local content from index server and quit program
                for(j = 0; j < numOfLocalContent; j++){
                    deregisterContent(localContentName[j], 1);
                }
                quit = 1;
                break;
            default://Invalid choice
                printf("Invalid choice, try again\n");
        } 
    }
    //Close UDP and TCP sockets; quit program
    close(s_udp);
    close(s_tcp);
    exit(0);
}
