/*================================================================
|                 CHAT APPLICATION - SERVER SIDE                 |
|                                                                |
| Name: server3.c                                                |
|                                                                |
| Written by: Itzel Acevedo Cardenas and                         |
|             Nikolas Gordon - September 2019                    |
|             Last update - December 2019                        |
|                                                                |
| Purpose: As a server, accept clients, receive their messages,  |
|          and send them to other clients using sockets          |
|                                                                |
| Usage: ./server3                                               |
|                                                                |
| Description of parameters: There are not any parameters needed |
|                                                                |
| Subroutines/Libraries required:                                |
|     See included statements.                                   |
|     For compile do gcc server3.c -o server3                    |
================================================================*/

/***************************LIBRARIES*****************************/
#include <stdio.h>                           //Needed for inputs and outputs
#include <sys/types.h>                       //Contains definitions of a number of data types used in system calls
#include <sys/socket.h>                      //Definitions of structures needed for sockets
#include <netinet/in.h>                      //Contains constants and structures needed for internet domain addresses
#include <stdlib.h>                          //Neededto use exit
#include <strings.h>                         //Needed to use bzero
#include <string.h>                          //This file contains funcions as strcpy and strcmp

/**************************GLOBAL VARIABLES***********************/
int clientCounter = 0;                       //Counter for clients
int clientSocket[5] = {0,0,0,0,0};           //This array will contain the socket number of clients 
int snumber = 1;                             //Counter of packages
float sversion = 3.0;                        //Version number

/*********************CLIENT USERNAMES STRUCTURE******************/
struct username {
    char Name[24];
} clientName[5];                             //This structure will be used as an array of strings that will contain the usernames

/***********************PACKET STRUCTURE***************************/
struct packet {
    int number;           //Packet number
    float version;        //Software version (3.0)
    char encryption[9];   //Encryption identifier (ROT13 or ClearText)
    int checksum;         //Checksum number
    char source[24];      //Sender (Username or Server)
    char destination[24]; //Receiver (Username, WHO, ALL, BYE)
    char type[8];         //Type of packet (MESSAGE, LIST, BYE)
    char data[256];       //Data part of the message
} serverPkt;              //This structure will contain the packages

/****************************FUNCTIONS******************************/
void sendMessage (int);                      //Function prototype to determine where to send the message
void quit(int);                              //Function prototype to disconnect a client
void sendAll (int);                          //Function prototype to send the message to everybody who is connected
void whoList(int);                           //Function prototype to send a list of connected clients
void individual(int);                        //Function prototype to send the message to one specific client
int comprobarSuma();                         //Function prototype to check if the bytes  agree with the checksum number in the header
void askRebroadcast(int);                    //Function prototype to ask for rebroadcast the message
void error(char *msg)                        //Function to exit if there is a problem
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     /********************CREATING THE SOCKET***********************/
     int sockfd, newsockfd, portno, clilen;                    //Variables to save the server socket, temporary client socket, number of the port, and size of the client address structure
     char user[24];                                            //String to temporary save the username
     struct sockaddr_in serv_addr, cli_addr;                   //Structures needed to create the socket
     int n;                                                    //Auxiliary variable that saves if there is a mistake on read or write

     sockfd = socket(AF_INET, SOCK_STREAM, 0);                 //The server socket is created
     if (sockfd < 0) error("ERROR opening socket");            //Exit if there were a problem creating the server socket
     bzero((char *) &serv_addr, sizeof(serv_addr));            //sockaddr_in structure is cleaned
     portno = 52378;                                           //Port number is established
     serv_addr.sin_family = AF_INET;                           //Structure family is set
     serv_addr.sin_addr.s_addr = inet_addr("192.197.151.116"); //Server IP address
     serv_addr.sin_port = htons(portno);                       //Port number
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)                          //The server socket is bind with the structure 
              error("ERROR on binding");                       //If there is a mistake, exit
     listen(sockfd,5);                                         //Only 5 clients can be connected at the same time
     clilen = sizeof(cli_addr);                                //clien variable saves cli_addr size

     /***************CONECTION WITH THE FIRST CLIENT******************/
     printf("Waiting for client 1 \n");
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //The server accepts the first client, and its socket number is tamporary saved in newsockfd
     if (newsockfd < 0) error("ERROR on accept");              //If there were a mistake on accepting client, exit
     clientSocket[clientCounter] = newsockfd;                  //The client socket is saved in the client socket array
     n = read (newsockfd,user,24);                             //Read socket to know the username
     if (n < 0) error("ERROR on username");                    //If there is an error reading username, exit
     strcpy(clientName[clientCounter].Name,user);              //Save the usename in the array of structures
     printf("%s has connected\n",clientName[clientCounter].Name); 

     clientCounter++;                                          //Client counter is incremented because a new client was added

     /***************CONECTION WITH THE SECOND CLIENT*****************/
     printf("Waiting for client 2\n");                         //Same instructions to add a second client
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0) error("ERROR on accept");                
     clientSocket[clientCounter] = newsockfd;
     n = read (newsockfd,user,24);
     if (n < 0) error("ERROR on username");
     strcpy(clientName[clientCounter].Name,user);
     printf("%s has connected\n",clientName[clientCounter].Name);

     clientCounter++;
     
     int i;
     while(1)                                                       //Infinite loop for the server to do the same until Ctrl+C
     {
         for(i = 0;i < 5; i++)                                      //Loop that go over all the 5 clients
         {
             if(clientSocket[i]!=0)                                 //If there is a client in that position of the array, do the following
             {
                 /****************READING FROM CLIENTS****************/
                 printf("Reading from %s\n",clientName[i].Name);
                 bzero(&serverPkt,sizeof(serverPkt));               //Server package is cleaned
                 n = read(clientSocket[i],&serverPkt,sizeof(serverPkt)); //Read the actual client socket and save it on the server package
                 if (n < 0) error("ERROR reading from socket");     //If there is a error reading socket, exit

                 /***********************CHECKSUM*********************/
                 if(comprobarSuma() == 0)                           //If the function comprobarSuma returns 0 means the packet was corrupted
                 {
		     printf("Message corrupted\n");                 //Print "Message corrupted"
		     askRebroadcast(i);                             //Ask the client for rebroadcast the same packet
		 }

                 /*********************CLIENT QUITS*******************/
                 if(strcmp(serverPkt.destination,"BYE") == 10)      //If the destination is BYE, go to the function quit(int)
                     quit(i);

                 /*******************SENDING MESSAGE******************/
                 else                                               //If not, go to the function sendMessage(int)
                     sendMessage(i);
             }
         }
     }
     return 0;
}



void sendMessage (int sender)                           //Function created to determine where to go depending on package destination
{
     if (strcmp(serverPkt.destination,"ALL") == 10)     //If the destination is ALL, go to the function sendAll(int)
         sendAll(sender);
     else 
         if (strcmp(serverPkt.destination,"WHO") == 10) //If the destination is WHO, go to the function whoList(int) 
             whoList(sender);
         else                                           //If the destination weren't any of the above go to the function individual(int)
             individual(sender);
}

void quit(int sender)                                             //Function to disconnect a client
{
    int n;                                                        //Auxiliary variable to save if there is a mistake
    printf("%s has desconnected\n",clientName[sender].Name);
    bzero(&serverPkt,sizeof(serverPkt));                          //The server package is cleaned
    serverPkt.number = snumber;                                   //The number of the package is saved
    snumber++;                                                    //The variable that saves the package number, is incremented
    serverPkt.version = sversion;                                 //The version is established
    strcpy(serverPkt.source,"Server");                            //The new source is the server
    strcpy(serverPkt.destination,clientName[sender].Name);        //The new destination is the sender
    strcpy(serverPkt.type,"BYE");                                 //Still the type of the package is BYE
    strcpy(serverPkt.data,"You have disconnected");               //The new data in the package is the noticed that the client is now disconnected
    n = write(clientSocket[sender],&serverPkt,sizeof(serverPkt)); //The package is written in the sender client socket
    if (n < 0) error("ERROR writing on socket");                  //If there were a mistake sending the message, exit
    clientSocket[sender] = 0;                                     //The socket number of the client is turned to 0 which means that there is not client in that position
    memset(clientName[sender].Name,0,sizeof(clientName[sender].Name));
    clientCounter--;                                              //The client counter is decreased
}

void sendAll (int sender)                                         //Function to send the message to all the connected clients
{
    int i, n;
    for (i = 0;i < 5;i++)                                         //This loop goes over all the posible clients
        if(clientSocket[i] != 0 && i != sender)                   //If there is a client in that position and the client is not the same sender do the following
        {
            serverPkt.number = snumber;                           //Set the new number of the package
            snumber++;                                            //Increment the variable of packages numbers
            n = write(clientSocket[i],&serverPkt,sizeof(serverPkt)); //Write on the respective client
            if (n < 0) error("ERROR on sending ALL");             //If there were a mistake on sending, exit
        }
    printf("Message sent from %s to All \nMessage: %s",clientName[sender].Name,serverPkt.data); 
    bzero(&serverPkt,sizeof(serverPkt));                          //The server package is cleaned
    serverPkt.number = snumber;                                   //The package is filled with the new information
    snumber++;
    serverPkt.version = sversion;
    strcpy(serverPkt.source,"Server");
    strcpy(serverPkt.destination,clientName[sender].Name);
    strcpy(serverPkt.type,"MESSAGE");
    strcpy(serverPkt.data,"Message sent to ALL");
    n = write(clientSocket[sender],&serverPkt,sizeof(serverPkt)); //The new package is sent to the original sender 
    if (n < 0) error("ERROR on sending ALL");                     //If there were a mistake sending, exit
}

void whoList(int sender)                                          //Function to send the list of connected clients
{
    int i,n;
    bzero(&serverPkt,sizeof(serverPkt));                          //The e server package is cleaned
    serverPkt.number = snumber;                                   //The package is filled with the new information
    snumber++;
    serverPkt.version = sversion;
    strcpy(serverPkt.source,"Server");
    strcpy(serverPkt.destination,clientName[sender].Name);
    strcpy(serverPkt.type,"LIST");
    for (i = 0;i < 5;i++)                                         //This loop goes over all possible clients
        if(clientSocket[i] != 0)                                  //If the socket is not empty add the name to the data part of the package
        {
            strcat(serverPkt.data," \n");
            strcat(serverPkt.data,clientName[i].Name);
        }
    n = write(clientSocket[sender],&serverPkt,sizeof(serverPkt)); //Write the package on the sender client
    if (n < 0) error("ERROR sending list");                       //If there were a mistake sending, exit
    printf("List sent to %s\n",clientName[sender].Name);
}

void individual(int sender)                                       //Function to send the package just to one client
{
    int n,receiver,j,aux = 0;
    for (receiver = 0;receiver < 5;receiver++)                    //Loop that goes over all possible clients
    {
        if (clientSocket[receiver]!=0)                            //If there is a client in that position of the array, do the following
        {    
            n = strcmp(serverPkt.destination, clientName[receiver].Name); //Compare the package destination with the name of the client and save the answer on the auxiliary variable n
	    if (n == 0 || n == 10)                                //If n == 0, means the strings are the same. If n == 10, means that the strings are the same but one of them have a \n at the end
            {
                aux = 1;                                          //Save a number 1 in the auxiliary variable aux, it is going to be use later
	        break;                                            //If the program got here, that means that the client was found, so it is not necessary to keep doing the loop, so break
            }
        }
    }
    if (aux == 1)                                                 //If the variable aux == 1 means that the receiver client was found so do the following to send the message
    {
        serverPkt.number = snumber;                               //Add the number to the package
        snumber++;                                                //Increase the snumber variable to be prepare to following packages
        n = write(clientSocket[receiver],&serverPkt,sizeof(serverPkt)); //Send the package to the receiver client
        if (n < 0) error("ERROR writing to socket");              //If there were a mistake sending the mackage, exit
        printf("Message sent from %s to %s \nMessage: %s",clientName[sender].Name,clientName[receiver].Name, serverPkt.data);

        bzero(&serverPkt,sizeof(serverPkt));                      //Clean the package structure 
        serverPkt.number = snumber;                               //Prepare the package again
        snumber++;
        serverPkt.version = sversion;
        strcpy(serverPkt.source,"Server");                        //Now the source is the server
        strcpy(serverPkt.destination,clientName[sender].Name);    //And the sender becomes the receiver
        strcpy(serverPkt.type,"MESSAGE");
        strcpy(serverPkt.data,"Message sent");                    //In the data part write that the message was sent
        n = write(clientSocket[sender],&serverPkt,sizeof(serverPkt)); //Write the package in the client socket
        if (n < 0) error("ERROR confirmating the message send");  //If there were a mistake writing, exit

    }
    else
        printf("Incorrect username or not mesagge\n");            //If the aplication got here, means that the username was incorrect or the message was empty
}

int comprobarSuma()                                          //Function to evaluate if the bytes of the data part match with the checksum number in the header
{
    int i,sum = 0;
    for (i = 0;serverPkt.data[i] != '\0';i++)                //Loop to pass from all the characters until the last one
        sum+=serverPkt.data[i];                                               //Variable to count the bytes, because each character means 1 byte
//    printf("Header: %i, Suma: %i\n",serverPkt.checksum,sum); Prints the checksum number in the header and the sum only to see if the application it is doing what it is supposed to do
    if(serverPkt.checksum == sum)                            //If the checksum match return 1
        return 1;
    else                                                     //If it does not match return 0
        return 0;
}

void askRebroadcast(int sender)                              //Function to ask for rebroadcast when the packet was corrupted
{
    int n;
    if(strcmp(serverPkt.destination,"READ")!=10)             //If the destination is READ dont ask again for the message because the socket has already something writen so the sender will receive that and no the resend ask so it wont know that it has to send it again and the application will break
    {
        bzero(&serverPkt,sizeof(serverPkt));                      //Clean the package structure 
        serverPkt.number = snumber;                               //Prepare the package again
        snumber++;
        serverPkt.version = sversion;
        strcpy(serverPkt.source,"Server");                        //Now the source is the server
        strcpy(serverPkt.destination,clientName[sender].Name);    //And the sender becomes the receiver
        strcpy(serverPkt.type,"RESEND");                          //The type is RESEND to identify the message
        strcpy(serverPkt.data,"Message corrupted");                   //In the data part write that the message was corrupted
        n = write(clientSocket[sender],&serverPkt,sizeof(serverPkt)); //Write the package in the client socket
        if (n < 0) error("ERROR confirmating the message send");      //If there were a mistake writing, exit
//        printf("Client asked for rebroadcast\n");                     Print only to know that the message was asked to be resend
        bzero(&serverPkt,sizeof(serverPkt));                          //Server package is cleaned
        n = read(clientSocket[sender],&serverPkt,sizeof(serverPkt));  //Read the actual client socket and save it on the server package
        if (n < 0) error("ERROR reading from socket");                //If there is a error reading socket, exit
    }
}
