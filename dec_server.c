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
void decrypt(char *, char *, int);
void streamManage(int, char *, int, char *);
int modulo(int x, int N);
void error(const char *msg);
void setupAddressStruct(struct sockaddr_in *, int);
int clientAuthorized(int, char *);
void parse(char *, char *, char *, char *);


/*******************************************************************************
**  main function
*******************************************************************************/
int main(int argc, char *argv[])
{
   int listenSocket, connSocket;
   socklen_t sizeOfClientInfo;
   struct sockaddr_in servAddr, clientAddr;

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

   const int opt = 1;
   setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

   // Enable the socket to begin listening
   if (bind(listenSocket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
      error("ERROR on binding");

   listen(listenSocket, 5); // listen to 5 connections or less
   int childExited; // child's exit status
   pid_t childPid; // process id for child
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

            // receive data stream
            streamManage(connSocket, combined, ssize, "recv");

            // parse data stream
            parse(combined, filename, key, txt);

            // make sure authorized client is making request
            if (!clientAuthorized(connSocket, filename))
               exit(1);

            // send encrypted text to client
            decrypt(txt, key, connSocket);
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
   if (strcmp(fname, "dec_client.c") != 0)
   {
      char badClient[50] = "enc_client cannot use dec_server";
      int flen = strlen(badClient);
      send(connSocket, &flen, sizeof(flen), 0);
      streamManage(connSocket, badClient, strlen(badClient), "send");
      close(connSocket);
      return 0;
   }
   return 1;
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
         strcpy(filename, token);

      if (i == 1)
         strcpy(key, token);

      if (i == 2)
         strcpy(text, token);

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
**  takes in a message and key, decrypts it and sends it to client
*******************************************************************************/
void decrypt(char *txt, char *key, int connSocket)
{
   char decrypted[strlen(txt)];
   memset(decrypted, '\0', sizeof(decrypted));

   for (int i = 0; i < strlen(txt); i++)
   {
      int decryptedChar; // holds a decrypted piece of the message
      int textChar = (int) txt[i]; // holds piece of the message
      int keyChar = (int) key[i]; // holds a piece of the key

      if (textChar == 32) // check if char is a space
         textChar = 64; // convert it to @

      if (keyChar == 32) // check if char is a space
         keyChar = 64; // convert it to @

      int sum = (textChar - 64) - (keyChar - 64);

      if (sum < 0) // use mod for values less than 0
      {
         decryptedChar = modulo(sum, 27) + 64;
      }
      else // use sum
      {
         decryptedChar = sum + 64;
      }

      if (decryptedChar == 64) // check if char is a @
      {
         decryptedChar = 32; // convert it to space
      }
      decrypted[i] = (char) decryptedChar; // add decrypted piece to whole
   }

   // send decrypted message to client
   int dsize = strlen(decrypted);
   send(connSocket, &dsize, sizeof(dsize), 0);
   streamManage(connSocket, decrypted, strlen(decrypted), "send");
   close(connSocket);
}

/*******************************************************************************
**  get modulus of negative number https://stackoverflow.com/questions/
 *  11720656/modulo-operation-with-negative-numbers
*******************************************************************************/
int modulo(int x, int N)
{
   return (x % N + N) % N;
}
/*******************************************************************************
**  shows error message without printing to stdrr
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

