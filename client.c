/*======================================================
|            CHAT APPLICATION - CLIENT SIDE            |
|                                                      |
| Name: client.c                                       |
|                                                      |
| Written by: Itzel Acevedo Cardenas - September 2019  |
|             Last update - December 2019              |
|                                                      |
| Purpose: As a client, connect to the server,         |
|          and send and receive messages using sockets |
|                                                      |
| Usage: ./client #username                            |
|        e.g. ./client2 itzel                          |
| Description of parameters:                           |
|     argv[1] - client username, it can be optional,   |
|     but if you don't type it there, It is going      |
|     to be asked later                                |
|                                                      |
| Subroutines/Libraries required:                      |
|     See included statements.                         |
|     To compile do gcc client.c -o client             |
======================================================*/  

/***************************LIBRARIES*****************************/
#include <stdio.h>      //Needed for inputs and outputs
#include <sys/types.h>  //Contains definitions of a number of data types used in system calls
#include <sys/socket.h> //Definitions of structures needed for sockets
#include <netinet/in.h> //Contains constants and structures needed for internet domain addresses
#include <netdb.h>      //This library contain functions as h_addr
#include <strings.h>    //Needed to use bzero
#include <stdlib.h>     //Neededto use exit
#include <string.h>     //This file contains funcions as strcpy and strcmp

/***********************PACKET STRUCTURE***************************/
struct packet {
    int number;           //Packet number
    float version;        //Software version (3.0)
    char encryption[9];   //Encryption identifier (ROT13 or ClearText)
    int checksum;         //Checksum number
    char source[24];      //Sender (Username)
    char destination[24]; //Receiver (Username, WHO, ALL, BYE)
    char type[8];         //Type of packet (MESSAGE, LIST, BYE)
    char data[256];       //Data part of the message
} clientPkt, receivedPkt; //Structures that build and receive the packages 

/****************************FUNCTIONS******************************/
void suma();          //Function prototype to make the sum of the bytes of the data part
void rot13(int);      //Function prototype to encrypt data if required
void error(char *msg) //Function to exit if there is a problem
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    /*****************CONECCTING WITH THE SERVER********************/
    int sockfd, portno, n;                         //Variables needed to save the socket number, the port number and the response of writing and reading from socket

    struct sockaddr_in serv_addr;                  //Structure needed to connect to the server
    struct hostent *server;                        //Needed structure where the server information will be copied

    portno = 52378;                                //Assigning the port number
    sockfd = socket(AF_INET, SOCK_STREAM, 0);      //The socket is created
    if (sockfd < 0) error("ERROR opening socket"); //If there were a mistake creating the socket, exit
    server = gethostbyname("loki.trentu.ca");      //The server IP is got and saved on the hostent structure
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");   //If there is nothing in the server variable, there is a mistake, so exit
        exit(0);
    }                                                   
    bzero((char *) &serv_addr, sizeof(serv_addr)); //sockaddr_in structure is cleaned
    serv_addr.sin_family = AF_INET;                //Address family, internet
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);                        //IP address is copied from hostent structure to sockaddr_in structure
    serv_addr.sin_port = htons(portno);            //Port number is added to the sockaddr_in structure
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //The client tries to connect
        error("ERROR connecting");                 //If there is a mistake connecting, exit
    
    /*******************GETTING THE USERNAME***********************/
    char username[24];                           //Variable used to save temporarily the username
    if(argc!=2) {                                //If the client did not type his username while running the application, ask him for it
        printf("What is your username?\n");
        fgets(username,24,stdin);
    }                                         
    else
        strcpy(username,argv[1]);                //If he typed his username on the terminal, just copy it to the username variable
    n = write(sockfd,username,strlen(username)); //The username is sent to the server
    if (n < 0)
        error("ERROR sending username");         //If there were a mistake sending the username, exit
    
    /********ASKIG TO TURN ON OR TURN OFF THE ENCRYPTION************/
    char cifrado[9];                                                            //Variable to save if the user wants or not encryption                                                                
    do{
        printf("Do you want encryption security? (y/n): ");                     //Ask if the user wants encryption security, to turn off or turn on the encryption
        fgets(cifrado,9,stdin);
        if (strcmp(cifrado,"y") == 10 || strcmp(cifrado,"Y") == 10)
            strcpy(cifrado,"ROT13");                                            //If the user wants encryption security, save ROT13 in cifrado variable
        else if (strcmp(cifrado,"n") == 10 || strcmp(cifrado,"N") == 10)
            strcpy(cifrado,"ClearText");                                        //If the user does not want encryption security, save ClearText in cifrado variable 
        else
            printf("Wrong answer, please only type y or n\n");                  //Notify the user that he or she wrote a wrong answer 
    } while (strcmp(cifrado,"ROT13") != 0 && strcmp(cifrado,"ClearText") != 0); //Keep asking until the user gives an valid answer

    int pktNumber = 1; //The package number is initialized
    int pkt;           //This variable is used to know if the actual packet is sent or received, it is needed fo the encryption process
    int salir=0;       //Auxiliary variable to know when the client wants to disconnect
    while(salir==0)    //Loop to send and receive messages until the client wants to disconnect
    {
        /****************THE PACKET IS CREATED*********************/
        bzero(&clientPkt,sizeof(clientPkt));           //The package structure is cleaned
        clientPkt.number = pktNumber;                  //The packet number is added
        pktNumber++;                                   //The variable that saves the packet number is incremented by one
        clientPkt.version = 3.0;                       //The version of the package is added
        strcpy(clientPkt.encryption,cifrado);          //The encryption flag is added
        strcpy(clientPkt.source,username);             //The client username is copied to the source part of the package
        printf("\nDestination: ");
        fgets(clientPkt.destination,sizeof(clientPkt.destination),stdin); //The destination is asked and added to the package
        if (strcmp(clientPkt.destination,"BYE") == 10) //The destination is compared to BYE if the result is 10, it means that the strings are the same, but one of them has a an extra \n at the end, which happens when you read with fget like this code does
        {
            strcpy(clientPkt.type,"BYE");              //If the code gets here, it means the client wants to disconnect
            salir = 1;                                 //The variable changes to break the loop
        }
        else
            if (strcmp(clientPkt.destination,"WHO") == 10) //If the destination is WHO then the package type is LIST
                strcpy(clientPkt.type,"LIST");
            else
                strcpy(clientPkt.type,"MESSAGE");      //Any other destination means that the peckage type is MESSAGE
        printf("Message: ");
        fgets(clientPkt.data,sizeof(clientPkt.data),stdin); //The data part is read

        /*************THE ENCRYPTION IS IMPLEMENTED***************/
        if(strcmp(cifrado,"ROT13") == 0) //If the user decided to encrypt his messages, then encrypt
        {
            pkt = 0;                     //This line identify the packet as an sending packet
            rot13(pkt);                  //Use ROT13 method to encrypt
        }

        /**********************CHECKSUM***************************/
	srand48(time(NULL));
        float numrand = drand48();  //Generate a random float number
//        printf("%f\n",numrand);       Printf the number only to see if the message should be corrupted or not
        if(numrand > 0.9)           //If the random number is lager than 0.9
            clientPkt.checksum = 0; //Corrupt the packet changing the checksum number to an incorrect number
        else
            suma();                 //If the packet shouldnt be corrupted calculate de correct checksum number

        /*******************SEND THE MESSAGE*********************/
        n = write(sockfd,&clientPkt,sizeof(clientPkt));     //The package is sent to the server
        if (n < 0) error("ERROR writing to socket");        //If there were a mistake sending the package, exit

        /*****************RECEIVE THE MESSAGE********************/
        bzero(&receivedPkt,sizeof(receivedPkt));            //The package structure is cleaned
        n = read(sockfd,&receivedPkt,sizeof(receivedPkt));  //The client reads from the server
        if (n < 0) error("ERROR reading from socket");      //If there were a mistake reading, exit
	
        /***************RESEND THE CORRUPTED MESSAGE*************/
        if(strcmp(receivedPkt.type,"RESEND") == 0)              //If theserver asked to rebroadcast the message do the following
        { 
 	    suma();                                             //Calculate the correct sum of the bytes
	    n = write(sockfd,&clientPkt,sizeof(clientPkt));     //The package is sent to the server again
            if (n < 0) error("ERROR writing to socket");        //If there were a mistake sending the package, exit
            bzero(&receivedPkt,sizeof(receivedPkt));            //The package structure is cleaned
            n = read(sockfd,&receivedPkt,sizeof(receivedPkt));  //The client reads from the server
            if (n < 0) error("ERROR reading from socket");      //If there were a mistake reading, exit
	}

        /*************DECRYPT THE RECEIVED MESSAGE***************/
        if(strcmp(receivedPkt.encryption,"ROT13") == 0) //If the message is encrypted, decrypt it
        {
            pkt = 1;                                        //This line identify the packet as an received packet
            rot13(pkt);                                 //Use ROT13 method to decrypt
        }

        /*************DISPLAY THE MESSAGE RECEIVED****************/
        printf("\nFrom: %s\nMessage: %s\n",receivedPkt.source,receivedPkt.data); //The received message is printed 
    }

    return 0;
}

void suma()                                   //Function to calculate the number of bytes of the data part of the packet
{
    int i, sum = 0;
    for (i = 0;clientPkt.data[i] != '\0';i++) //Loop to go from the first character until the last one
        sum+=clientPkt.data[i];               //Variable that saves the number of characters that is equal to the number of bytes because each character needs a byte
//    printf("Suma: %i\n",sum);                   sum number printed only to see if a correct sum was made
    clientPkt.checksum = sum;                 //The checksum number is added to the header 
}

void rot13(int pkt)                                //Function to implement ROT13 method
{
    int c,i;                                       //Auxiliary variables c for characters and i for interations in the loop 
    if (pkt == 0)                                  //if pkt = 0, it means the packet that needs encryption, is the clientPkt
        for (i = 0;clientPkt.data[i] != '\0';i++)  //This loop goes from the first character on the string until the last one
        {
            c = clientPkt.data[i];                 //c is equal to the ascii value of the letter
            if (c >= 'A' && c < 'N')               //If the letter is between A and M, add 13 to the ascii value
                clientPkt.data[i] = c + 13;
            else if (c >= 'N' && c <= 'Z')         //If the letter is between N and Z, subtract 13 to the ascii value
                clientPkt.data[i] = c - 13;
            else if (c >= 'a' && c < 'n')          //Do the same with the lowercase
                clientPkt.data[i] = c + 13;
            else if (c >= 'n' && c <= 'z')
                clientPkt.data[i] = c - 13;
        }
    else                                            //if pkt = 1, it means the packet that needs encryption, is the receivedPkt
        for (i = 0;receivedPkt.data[i] != '\0';i++) //Do the same loop with the receivedPkt
        {
            c = receivedPkt.data[i];
            if (c >= 'A' && c < 'N')
                receivedPkt.data[i] = c + 13;
            else if (c >= 'N' && c <= 'Z')
                receivedPkt.data[i] = c - 13;
            else if (c >= 'a' && c < 'n')
                receivedPkt.data[i] = c + 13;
            else if (c >= 'n' && c <= 'z')
                receivedPkt.data[i] = c - 13;
        }
}
