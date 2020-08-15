/*******************************************************************************
** Program:       Assignment 4 - CS344
** Author:        David Vega
** Date:          8/1/20
** Description:   In this assignment, you will be creating five small programs
**                that encrypt and decrypt information using a one-time
**                pad-like system. These programs will combine the
**                multi-processing code you have been learning with socket-based
**                inter-process communication. Your programs will also be
**                accessible from the command line using standard Unix features
**                like input/output redirection, and job control. Finally, you
**                will write a short compilation script.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

/*******************************************************************************
**  Function declarations
*******************************************************************************/
void encrypt(char *, char *, int);
void streamManage(int, char *, int, char *);
void error(const char *msg);
void setupAddressStruct(struct sockaddr_in *, int);
int clientAuthorized(int, char *);
void parse(char *, char *, char *, char *);

/*******************************************************************************
**  main function
*******************************************************************************/
int main(int argc, char *argv[])
{
   int listenSocket, connSocket; // main socket and connection socket
   socklen_t sizeOfClientInfo;
   struct sockaddr_in servAddr, clientAddr; // for server and client

   if (argc < 2)
   {
      fprintf(stderr, "USAGE: %s port\n", argv[0]);
      exit(1);
   }

   // Set up the address struct for the server socket
   setupAddressStruct(&servAddr, atoi(argv[1]));


   // Set up the socket
   listenSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (listenSocket < 0)
      error("ERROR opening socket");

   // make ports reusable
   const int reuse = 1;
   setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));


   // Enable the socket to begin listening
   if (bind(listenSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
      error("ERROR on binding");

   // set the socket to listen and accept 5 connections at once
   listen(listenSocket, 5);

   int childExited; // child's exit status
   pid_t childPid; // process id for child

   // set buffers
   char txt[75000], key[75000], filename[75000], combined[170000];

   while (1)
   {
      childPid = fork(); // a baby is born
      switch (childPid)
      {
         case -1: // still birth
         {
            printf("Could not create child process\n");
         }
         case 0: // birth successful
         {
            // clear char arrays
            memset(txt, '\0', sizeof(txt));
            memset(key, '\0', sizeof(key));
            memset(filename, '\0', sizeof(filename));
            memset(combined, '\0', sizeof(combined));


            // get client info and accept connection
            sizeOfClientInfo = sizeof(clientAddr);
            connSocket = accept(listenSocket,
                                (struct sockaddr *) &clientAddr,
                                &sizeOfClientInfo);
            if (connSocket < 0) // connection failed
               error("ERROR on accept");

            // get stream size
            int ssize;
            recv(connSocket, &ssize, sizeof(ssize), 0);

            // receive txt
            streamManage(connSocket, combined, ssize, "recv");

            // parse data from client
            parse(combined, filename, key, txt);

            // make sure authorized client is making request
            if (!clientAuthorized(connSocket, filename))
               exit(1);

            // encrypt data and send it back to the client
            encrypt(txt, key, connSocket);
            return 0;
         }
         default:
         {
            waitpid(-1, &childExited, 0);
         }
      }
   }
   close(listenSocket); // Close the listening socket
   return 0;
}

/*******************************************************************************
**  check if client is accessing the correct server
*******************************************************************************/
int clientAuthorized(int connSocket, char *fname)
{
   if (strcmp(fname, "enc_client.c") != 0) // needs matching client
   {
      char badClient[50] = "dec_client cannot use enc_server";
      int bad = strlen(badClient);
      send(connSocket, &bad, sizeof(bad), 0); // send message size first
      streamManage(connSocket, badClient, strlen(badClient), "send");
      close(connSocket);
      return 0; // client did not match return that in main
   }
   return 1; // client matched all is clear
}
/*******************************************************************************
**  takes in a message and key, encrypts it and sends it to client
*******************************************************************************/
void encrypt(char *txt, char *key, int connSocket)
{
   char encrypted[strlen(txt)];
   memset(encrypted, '\0', sizeof(encrypted));

   for (int i = 0; i < strlen(txt); i++)
   {
      int encryptedChar; // holds a encrypted piece of the message
      int textChar = (int) txt[i]; // holds piece of the message
      int keyChar = (int) key[i]; // holds a piece of the key

      if (textChar == 32) // check if char is a space
         textChar = 64; // convert it to @

      if (keyChar == 32) // check if char is a space
         keyChar = 64; // convert it to @

      int sum = (textChar - 64) + (keyChar - 64);

      if (sum > 26) // use mod for values greater than 26
      {
         encryptedChar = (sum % 27) + 64;
      }
      else // use sum for values less than 27
      {
         encryptedChar = sum + 64;
      }

      if (encryptedChar == 64) // check if char is @
         encryptedChar = 32; // convert it to space

      encrypted[i] = (char) encryptedChar; // add encrypted piece to whole
   }

   // send encrypted message to client
   int esize = strlen(encrypted);
   send(connSocket, &esize, sizeof(esize), 0);
   streamManage(connSocket, encrypted, strlen(encrypted), "send");
   close(connSocket);
}
/*******************************************************************************
**  send or receive one piece at a time until entire message is transported
*******************************************************************************/
void parse(char *buffer, char *filename, char *key, char *text)
{
   // parse data sent from client stored in buffer
   char *token = strtok(buffer, "#");
   int i = 0;
   while (token != NULL)
   {
      if (i == 0)
         strcpy(filename, token); // first chunk found

      if (i == 1)
         strcpy(key, token); // second chunk found

      if (i == 2)
         strcpy(text, token); // last chunk

      token = strtok(NULL, "#");
      i++;
   }
}

/*******************************************************************************
**  send or receive data and keep track of data not sent and request repeat
*******************************************************************************/
void streamManage(int conn, char *buffer, int size, char *type)
{
   int remaining = size; // what hasn't been received in stream yet
   int passed = 0; // what has been received in stream
   int chars; // chunk that was received

   while (remaining > 0) // loop as long as theres data to read/send
   {
      if (strcmp(type, "send") == 0) // send data
         chars = send(conn, buffer + passed, remaining, 0);
      if (strcmp(type, "recv") == 0) // receive data
         chars = recv(conn, buffer + passed, remaining, 0);

      if (chars == -1) // error
      {
         exit(1);
      }
      passed += chars; // add chunk to received
      remaining -= chars; // reduce remaining
   }
}

/*******************************************************************************
**  error message only prints in terminal but doesn't get piped
*******************************************************************************/
void error(const char *msg)
{
   perror(msg);
   exit(1);
}

/*******************************************************************************
**  Set up the address struct for the server socket
*******************************************************************************/
void setupAddressStruct(struct sockaddr_in *address, int portNumber)
{
   // Clear out the address struct
   memset((char *) address, '\0', sizeof(*address));

   // The address should be network capable
   address->sin_family = AF_INET;

   // Store the port number
   address->sin_port = htons(portNumber);

   // Allow a client at any address to connect to this server
   address->sin_addr.s_addr = INADDR_ANY;
}

