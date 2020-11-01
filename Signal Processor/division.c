///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        division.c
// This File:        division.c
// Other Files:      intdate.c, sendsig.c
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
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int count = 0;//the number of completed success

void SIGFPE_handler() {
  printf("Error: a division by 0 operation was attempted.\n");
  printf("Total number of operations successfully completed: %i\n", count);
  printf("The program will be terminated.\n");
  exit(0);
}

void  SIGINT_handler () {
  printf("\nTotal number of operations successfully completed: %i\n", count);
  printf("The program will be terminated.\n");
  exit(0);
}

/* PARTIALLY COMPLETED:
 * This program is used to produce signals and handle those signals
 *
 * argc: CLA count
 * argv: CLA value
 */
int main() {
  //create a sigaction for SIGINT signal
  struct sigaction act;
  memset (&act, 0, sizeof(act));
  //check this sigaction
	act.sa_handler = SIGINT_handler;
  if(sigaction(SIGINT, &act, NULL) != 0){
    printf("Handling is error");
    exit(1);
  }
  //create a sigaction for SIGFPE signal
  struct sigaction act2;
  memset (&act2, 0, sizeof(act));
  //check this sigaction
	act2.sa_handler = SIGFPE_handler;
  if(sigaction(SIGFPE, &act2, NULL) != 0){
    printf("Handling is error");
    exit(1);
  }

  //use a loop to prompt user inputs
	while(1) {
    printf("Enter first integer: ");
    char buf[100];

    //need to check fgets
    if(fgets(buf, 100, stdin) == NULL){
        printf("Error inputs ");
        exit(0);
    }

    int num1 = atoi(buf);
    printf("Enter second integer: ");
    char buf2[100];
    if(fgets(buf2, 100, stdin) == NULL){
        printf("Error inputs ");
        exit(0);
    }
    int num2 = atoi(buf2);
    //get the result and the reminder of the user inputs
    int result = num1 / num2;
    int reminder = num1 % num2;
    printf("%i / %i is %i with a remainder of %i\n", num1, num2, result, reminder);
    count++;//increase count by 1
	}
	return (0);
}
