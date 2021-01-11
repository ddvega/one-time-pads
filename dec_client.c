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

int contentValid(char *);

/*******************************************************************************
**  Main function
*******************************************************************************/
int main(int argc, char *argv[])
{
   //argv1 = plaintext, argv2 = key, argv3 =  port
   // check number of args
   if (argc < 3)
   {
      fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
      exit(0);
   }

   // store information about target server
   struct sockaddr_in serverAddress;
   setupAddressStruct(&serverAddress, atoi(argv[3]));

   // Set up socket to communicate with server
   int socketFD;
   socketFD = socket(AF_INET, SOCK_STREAM, 0);
   if (socketFD < 0)
      error("CLIENT: ERROR opening socket");

   // Connect socket to target server
   if (connect(socketFD,
               (struct sockaddr *) &serverAddress,
               sizeof(serverAddress)) < 0)
      error("CLIENT: ERROR connecting");

   // target server takes in a filename, key and message
   // target takes data and returns an encrypted message
   // setup buffers to store data being sent to server
   char key[75000], txt[75000], filename[500];
   char decrypted[75000], combined[170000];

   // initialize buffers
   memset(txt, '\0', sizeof(txt));
   memset(key, '\0', sizeof(key));
   memset(decrypted, '\0', sizeof(decrypted));
   memset(combined, '\0', sizeof(combined));

   // setup pointers to open files
   FILE *keyfile = fopen(argv[2], "r");
   FILE *plaintext = fopen(argv[1], "r");

   // copy contents of files to buffers
   fgets(key, 75000, keyfile);
   fgets(txt, 75000, plaintext);

   // close files
   fclose(keyfile);
   fclose(plaintext);

   // copy filename to filename buffer
   strcpy(filename, __FILE__); // store name of client

   // replace new line characters with terminators
   filename[strcspn(filename, "\n")] = '\0'; // terminate txt string
   key[strcspn(key, "\n")] = '\0'; // terminate txt string
   txt[strcspn(txt, "\n")] = '\0'; // terminate txt string

   // combine filename, key and text in one string
   sprintf(combined, "%s#%s#%s#", filename, key, txt);

   // check that the key is as large as text file
   if (strlen(key) < strlen(txt))
   {
      printf("Error: key %s is too short!\n", argv[2]);
      exit(1);
   }

   // send only valid characters to the server
   if(!contentValid(txt))
      error("dec_client error: input contains bad characters\n");

   // send message, key and filename to server
   // send length of message first
   int dataMessageSize = strlen(combined);
   send(socketFD, &dataMessageSize, sizeof(dataMessageSize), 0);

   // send message, key and filename to server
   streamManage(socketFD, combined, dataMessageSize, "send");

   // receive length of decrypted message size
   int decryptedMessageSize;
   recv(socketFD, &decryptedMessageSize, sizeof(decryptedMessageSize), 0);

   // receive decrypted message from server
   streamManage(socketFD, decrypted, decryptedMessageSize, "recv");

   // print decrypted message to screen
   printf("%s\n", decrypted);

   close(socketFD);
   return 0;
}
/*******************************************************************************
**  make sure that characters in message are valid
*******************************************************************************/
int contentValid(char *txt)
{
   // block messages that don't consist of ABCDEFGHIJKLMNOPQRSTUVWXYZ
   for (int i = 0; i < strlen(txt) - 1; i++)
   {
      if ((int) txt[i] == 32) // space character is ok
      {
         continue;
      } else if ((int) txt[i] < 65 ||
                 (int) txt[i] > 90) // needs to be uppercase
      {
         return 0;
      }
   }
   return 1;
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
**  process raw input and parse it into an input array used by execvp
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