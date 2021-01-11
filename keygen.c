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
#include <time.h>
#include <stdlib.h>

/*******************************************************************************
**  main function
*******************************************************************************/
int main(int argc, char *argv[])
{
   srand(time(0)); // seed rand function

   int keySize = atoi(argv[1]); // size of key used in loop


   if (keySize < 1) // invalid key size
   {
      fprintf(stderr, "Key size invalid. \n");
      exit(1);
   }

   char key[keySize+1]; // space to store key

   for (int i = 0; i < keySize; i++)
   {
      // get random upper case letter
      int c = 64 + (rand() % 27);

      if (c == 64) // convert 64 to 32 (@ -> ' ')
         c = 32;

      key[i] = (char)c; // append character to key

   }
   key[keySize] = '\n'; // terminate the string

   printf("%s", key); // print to screen

   return 0;
}
