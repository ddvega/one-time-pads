/*******************************************************************************
** Program:       Assignment 4
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
#include <sys/socket.h>
#include <netinet/in.h>

/*******************************************************************************
**  Function declarations
*******************************************************************************/
void streamManage(int, char *, int, char *);
void error(const char *msg);
void setupAddressStruct(struct sockaddr_in *, int);

/*******************************************************************************
**  Main function
*******************************************************************************/
int main(int argc, char *argv[])
{
   //argv1 = plaintext, argv2 = key, argv3 =  port
   int socketFD;
   struct sockaddr_in serverAddress;

   // check number of args
   if (argc < 3)
   {
      fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
      exit(0);
   }

   // Set up the server address struct
   setupAddressStruct(&serverAddress, atoi(argv[3]));

   // Set up the socket
   socketFD = socket(AF_INET, SOCK_STREAM, 0);
   if (socketFD < 0)
      error("CLIENT: ERROR opening socket");

   // Connect to server
   if (connect(socketFD,
               (struct sockaddr *) &serverAddress,
               sizeof(serverAddress)) < 0)
      error("CLIENT: ERROR connecting");

   // setup pointers to open plaintext and key files
   FILE *keyfile = fopen(argv[2], "r");
   FILE *plaintext = fopen(argv[1], "r");

   // setup buffers
   char key[75000], txt[75000], filename[500];
   char encrypted[75000], combined[170000];

   // initialize buffers
   memset(txt, '\0', sizeof(txt));
   memset(key, '\0', sizeof(key));
   memset(encrypted, '\0', sizeof(encrypted));
   memset(combined, '\0', sizeof(combined));

   // get contents of files
   fgets(key, 75000, keyfile);
   fgets(txt, 75000, plaintext);

   // close files
   fclose(keyfile);
   fclose(plaintext);

   // copy filename to filename buffer
   strcpy(filename, __FILE__); // store name of client

   // replace new line with terminators
   filename[strcspn(filename, "\n")] = '\0'; // terminate txt string
   key[strcspn(key, "\n")] = '\0'; // terminate txt string
   txt[strcspn(txt, "\n")] = '\0'; // terminate txt string

   // combine filename, key and text in one string
   sprintf(combined, "%s#%s#%s#", filename, key, txt);

   // check that key is as large as text file
   if (strlen(key) < strlen(txt))
   {
      printf("Error: key %s is too short!\n", argv[2]);
      exit(1);
   }

   // block input with bad characters
   for (int i = 0; i < strlen(txt) - 1; i++)
   {
      if ((int) txt[i] == 32) // space character is ok
      {
         continue;
      }
      else if ((int) txt[i] < 65 || (int) txt[i] > 90) // needs to be uppercase
      {
         error("enc_client error: input contains bad characters\n");
      }
   }

   // send data to server
   int length = strlen(combined);
   send(socketFD, &length, sizeof(length), 0);
   streamManage(socketFD, combined, length, "send");

   // receive encrypted data from server
   int encLen;
   recv(socketFD, &encLen, sizeof(encLen), 0);
   streamManage(socketFD, encrypted, encLen, "recv");

   // decrypted message printed to screen
   printf("%s\n", encrypted);

   close(socketFD);
   return 0;
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
**  print error message to terminal that won't get piped to a file
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