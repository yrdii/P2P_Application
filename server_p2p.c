#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define BUFLEN		101

struct tpacket {
  char type;
  char data[100];
};

typedef struct  {
  char contentName[10]; 
  char peerName[10];
  int host;
  int port;
} content_Listing;

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

struct T_pdu { 
  	char type;
  	char peerName[10];
  	char contentName[10];
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
 	char content_List[100]; 
};

struct E_pdu {
	char type;
 	char errMsg[100];
};

int main(int argc, char *argv[]) 
{
  // S_packet
  struct  R_pdu R_packet, *R_pkt;
  struct  S_pdu S_packet;
  struct  O_pdu O_packet;
  struct  T_pdu T_packet;
  struct  E_pdu E_packet;
  struct  A_pdu A_packet;
  
  int match = 0;
  int list_match = 0;
  int unique = 1;
  int list_pointer = 0;
  int end_Pointer = 0;
  
  // Arrays to store information about content and a list of content names
  content_Listing content_List[10];
  content_Listing ContentBlockTemp;
  char ContentNameList[10][10];

  // Transmission Variables
  struct  tpacket packet_receIved;
  struct  tpacket packet_sent;
  int     R_host, R_port;
  char tcp_port[6];
  char tcp_ip_addr[5];

  // Socket Primitives
  struct  sockaddr_in fsin;	        		/* the from address of a client	*/
	char    *pts;
	int		sock;			                      /* server socket */
	time_t	now;			                    /* current time */
	int		alen;			                      /* from-address length */
	struct  sockaddr_in sin;          		/* the from address of a client */
  int     s,sz, type;               		/* socket descriptor and socket type */
	int     port = 3000;              		/* default port */
	int 	counter, n,m,i,bytes_to_read,bufsize;
  int     error_flag = 0;

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}
    /* Socket Initialization */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;           //a IPv4 address
    sin.sin_addr.s_addr = INADDR_ANY;   //random IP address
    sin.sin_port = htons(port);         //bind to this port number
                                                                                                 
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
		  fprintf(stderr, "Cannot create socket\n");
                                                                                
    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		  fprintf(stderr, "Cannot bind to %d port: \n",port);
    listen(s, 5);	
	    alen = sizeof(fsin); 
    
    /* -------------------------------MAIN------------------------------------------*/
    while(1) {
      //continuously listens for S_packet, recvfrom recives packet from socket s and stores in packet_receIved
      if (recvfrom(s, &packet_receIved, BUFLEN, 0,(struct sockaddr *)&fsin, &alen) < 0)
	      fprintf(stderr, "ERROR: reciveing packet error\n");
	    
      switch(packet_receIved.type){
	
	      /* ---------------------------------Content Registration--------------------------------- */
        case 'R':
	        fprintf(stderr, "------Content Registration------\n"); 
          memset(&R_packet,'\0', sizeof(R_packet));
          for(i=0;i<10;i++){
	            //extracts info from received packet (packet_receIved.data) and stores in respective R_packet
              R_packet.peerName[i] = packet_receIved.data[i];       	//copies characters from index 0 to 9   
              R_packet.contentName[i] = packet_receIved.data[i+10]; 	//copies characters from index 10 to 19 
              if (i<5) {
                R_packet.host[i] = packet_receIved.data[i+20];      	//copies characters from index 20 to 24 (next 5 characters)
              }
              if (i<6) {
                R_packet.port[i] = packet_receIved.data[i+25];      	//copies characters from index 25 to 30 (next 6 characters) 
              }
          }
	        //convert array to int
          R_host = atoi(R_packet.host); 
          R_port = atoi(R_packet.port);
         
         error_flag=0; 
         for(i=0;i<end_Pointer;i++){
           fprintf(stderr, "Registered Info: %s %s %d %d\n", content_List[i].contentName, content_List[i].peerName,content_List[i].port, content_List[i].host);
           // Content has already been uploaded by Peer, set error_flag to 1
           if (strcmp(content_List[i].contentName, R_packet.contentName) == 0 && strcmp(content_List[i].peerName, R_packet.peerName) == 0) {
             error_flag=1;
             break;
           }
           // PeerName and Address Mismatch, set error_flag to 2
           else if (strcmp(content_List[i].peerName, R_packet.peerName)==0 && !(content_List[i].host == R_host)) {
             error_flag=2;
             break;
           }
         }

          /*  No errors so: Send Reply  */
          if (error_flag==0){

            // checks if the content is new 
            list_match=0;
            for(i=0;i<list_pointer;i++){
              if (strcmp(ContentNameList[i], R_packet.contentName) == 0){
                list_match = list_match+1;
                break;
              }
            }
            // if it is new, adds it to the content list (ContentNameList) 
            if (!list_match) {
              strcpy(ContentNameList[list_pointer],R_packet.contentName);
              list_pointer = list_pointer + 1;
            }

            // Commits content info to content_List
            memset(&content_List[end_Pointer],'\0', sizeof(content_List[end_Pointer]));
            strcpy(content_List[end_Pointer].contentName,R_packet.contentName);
            strcpy(content_List[end_Pointer].peerName,R_packet.peerName);
            content_List[end_Pointer].host = R_host;
            content_List[end_Pointer].port = R_port;

            fprintf(stderr, "Peer Name: %s\n", content_List[end_Pointer].contentName);
            fprintf(stderr, "Content Name: %s\n", content_List[end_Pointer].peerName);
            fprintf(stderr, "Port %d, Host %d\n", content_List[end_Pointer].port, content_List[end_Pointer].host);
            end_Pointer = end_Pointer +=1; // Increment pointer to NEXT block
            
            // Sends acknowledgment packet with peerName to peer if no errors
            packet_sent.type = 'A';
            memset(packet_sent.data, '\0', 100);
            strcpy(packet_sent.data, R_packet.peerName);
            fprintf(stderr, "Acknowledge\n"); 
          }
          else {
            // Send Error Packet to peer
            packet_sent.type = 'E';
            memset(packet_sent.data, '\0', 100);
            if (error_flag==2) {
		          strcpy(packet_sent.data,"ERROR: Peer name has already been registered");
            }
            else if (error_flag==1) {
              strcpy(packet_sent.data,"ERROR: File Already Registered");
            }
            fprintf(stderr,"ERROR\n");
          }
	        
          //packet (acknowledgment or error) sent to peer
          sendto(s, &packet_sent, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
	        
          //server logs the current state of the ContentNameList after processing the registration
          for(i=0;i<list_pointer;i++){
              fprintf(stderr, "Register File List: %s\n", ContentNameList[i]);
          }
          break;
        
        /* ---------------------------------Peer Server Content-Location Request--------------------------------- */
        case 'S':
	        fprintf(stderr, "------Content location request------\n"); 
          //Localize Content
          memset(&S_packet,'\0', sizeof(S_packet));
          for(i=0;i<10;i++){
              S_packet.peerName[i] = packet_receIved.data[i];       	        // Copying peerName from received data index 0 to 9
              S_packet.contentNameOrAddress[i] = packet_receIved.data[i+10];  // Copying contentNameOrAddress from received data from index 10 to 19
          }
	        // searches through list of registered content to find a match with requested content name/address
          match = 0;
          for(i=0;i<end_Pointer;i++){
            if (strcmp(S_packet.contentNameOrAddress, content_List[i].contentName) == 0){
	            // If a match is found, store the content information in ContentBlockTemp
              fprintf(stderr, "Search Match: %s %s %d %d\n", content_List[i].contentName, content_List[i].peerName,content_List[i].port, content_List[i].host);
              memset(&ContentBlockTemp,'\0', sizeof(ContentBlockTemp));
              strcpy(ContentBlockTemp.contentName, content_List[i].contentName);
              strcpy(ContentBlockTemp.peerName, content_List[i].peerName);
              ContentBlockTemp.host = content_List[i].host;
              ContentBlockTemp.port = content_List[i].port;
              match=1; 
            }
            // If match is found, corresponding entry in content_List is shifted down to simulate removing it from the list
            if (match && i < end_Pointer-1){
              strcpy(content_List[i].contentName,content_List[i+1].contentName);
              strcpy(content_List[i].peerName, content_List[i+1].peerName);
              content_List[i].host = content_List[i+1].host;
              content_List[i].port = content_List[i+1].port;
            }
          }

          if (match) {
	          //if match found in content list, server prepares response packet (packet_sent) with type S
            end_Pointer = end_Pointer-1;
            memset(&content_List[end_Pointer],'\0', sizeof(content_List[end_Pointer]));
            strcpy(content_List[end_Pointer].contentName,ContentBlockTemp.contentName);
            strcpy(content_List[end_Pointer].peerName,ContentBlockTemp.peerName);
            content_List[end_Pointer].host = ContentBlockTemp.host;
            content_List[end_Pointer].port = ContentBlockTemp.port;
            end_Pointer = end_Pointer+1;

            packet_sent.type = 'S';
            memset(packet_sent.data, '\0', 100);
            memset(tcp_ip_addr, '\0',sizeof(tcp_ip_addr));
            memset(tcp_port, '\0', sizeof(tcp_port));
            strcpy(packet_sent.data,ContentBlockTemp.peerName);
            snprintf (tcp_ip_addr, sizeof(tcp_ip_addr), "%d", ContentBlockTemp.host);
            snprintf (tcp_port, sizeof(tcp_port), "%d",ContentBlockTemp.port);
            memcpy(packet_sent.data+10, tcp_ip_addr, 5);
            memcpy(packet_sent.data+15, tcp_port, 6);
            }
          else {
            // If no match is found, send an error message
            packet_sent.type = 'E';
            memset(packet_sent.data, '\0', 100);
		        strcpy(packet_sent.data,"ERROR: Content was not found");
          }
	        
          //send response packet to peer
          sendto(s, &packet_sent, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;
        
        /* ---------------------------------Peer Server Content List Request--------------------------------- */
        case 'O':
	        fprintf(stderr, "------Content List Request------\n"); 
          // clear data field of packet_sent, to prepare it to be filled with new content list
          memset(packet_sent.data, '\0', 100);

	        //iterates through ContentNameList and copies content name into packet_sent
          for(i=0;i<list_pointer;i++){
	          //ContentNameList is array of registered content names
            memcpy(packet_sent.data+i*10, ContentNameList[i], 10);
            fprintf(stderr, "Registered Content Results: %s\n", packet_sent.data+i*10);
          }
	        //sends packet with list of content names to peer
          packet_sent.type = 'O';
          sendto(s, &packet_sent, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;
        
        /* ---------------------------------Peer Server Content De-registration--------------------------------- */
        case 'T':
	         fprintf(stderr, "------Content De-Registration------\n"); 
          //initialize T_packet and extract peerName and content
          memset(&T_packet,'\0', sizeof(T_packet));
          for(i=0;i<10;i++){
              T_packet.peerName[i] = packet_receIved.data[i];        // Copying peerName from received data from index 0 to 9
              T_packet.contentName[i] = packet_receIved.data[i+10];  // Copying contentName from received data from index 10 to 19
          }
          // Remove the Content from registration
          match = 0;
          for(i=0;i<end_Pointer;i++){
            if ((strcmp(T_packet.peerName, content_List[i].peerName) == 0) && strcmp(T_packet.contentName, content_List[i].contentName)==0){
              // If a match is found, mark it for removal and print information
	            match=1;
              fprintf(stderr, "File to be removed:  %s %s %d %d\n", content_List[i].contentName, content_List[i].peerName,content_List[i].port, content_List[i].host);
            }
            // If content is found, shift all content down queue, to simulate removing the content
            if (match && i < end_Pointer-1){
              strcpy(content_List[i].contentName,content_List[i+1].contentName);
              strcpy(content_List[i].peerName, content_List[i+1].peerName);
              content_List[i].host = content_List[i+1].host;
              content_List[i].port = content_List[i+1].port;
            }
          }

          if (match) {
            unique = 1; 
    	      // Check if the content name is unique, if so:
	          end_Pointer = end_Pointer -1; 
            for(i=0;i<end_Pointer;i++){
              if (strcmp(T_packet.contentName, content_List[i].contentName)==0){
                unique = 0;
              }
            }
            match = 0;
            if (unique){
	            //loop through IContentNameList to find position of content name that matches T_packet.contentName
              for(i=0;i<list_pointer;i++){
                if (strcmp(T_packet.contentName, ContentNameList[i])==0) {
		              //match has been found
                  match=1;
                }
                if (match && i < list_pointer-1){
		              //since match is found, shift elements to left (to remove matched content name)
                  strcpy(ContentNameList[i],ContentNameList[i+1]);
                }
              }
              list_pointer = list_pointer -1;
            }
	          // Prepare and send Acknowledgment Packet
            packet_sent.type = 'A';
            memset(packet_sent.data, '\0', 10);
            strcpy(packet_sent.data, T_packet.peerName);
          }
          else {
	          // If no match is found, send an error message
            packet_sent.type = 'E';
            memset(packet_sent.data, '\0', 10);
            strcpy(packet_sent.data, "ERROR: File Removal-no match was found");
          }
	        //send response packet to peer
          sendto(s, &packet_sent, BUFLEN, 0,(struct sockaddr *)&fsin, sizeof(fsin));
          break;

        default:
          break;
      } 

    }
}
