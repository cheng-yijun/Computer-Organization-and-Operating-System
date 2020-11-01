///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        sendsig.c
// This File:        sendsig.c
// Other Files:      intdate.c, division.c
// Semester:         CS 354 Fall 2019
//
// Author:           Yijun Cheng
// Email:            cheng229@wisc.edu
// CS Login:         yijunc
//
/////////////////////////// OTHER SOURCES OF HELP /////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
// Persons:          NONE
// Online sources:   NONE
//
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

/* PARTIALLY COMPLETED:
 * This program is used to produce signals to other process
 *
 * argc: CLA count
 * argv: CLA value
 */
int main(int argc, char *argv[]) {
  if (argc != 3){//check the number of input arguments
    //print the correct usage
    printf("Usage: <signal type> <pid>\n");
    exit(1);
  }
  //create variables of process id and signal types
  int pid = atoi(argv[2]);
  int type = strcmp(argv[1], "-u");
  int type2 = strcmp(argv[1], "-i");
  //check the signal type from user
  if (type == 0){//typr is SIGUSR1
     if(kill(pid, SIGUSR1) == -1){//type is SIGUSR1
       printf("error Handling SIGUSR1\n");
       exit(1);
     }
   }else if (type2 == 0){//type is SIGINT
     if(kill(pid, SIGINT) == -1){
       printf("error Handling SIGINT\n");
       exit(1);
     }
   }else{//cannot find this signal type
     printf("no such signal type\n");
     exit(1);
   }

   return 0;
}
