///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        intdate.c
// This File:        intdate.c
// Other Files:      sendsig.c, division.c
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

int alarm_time = 3;
int count = 0;

/*
  This is a alarm signal Handler
*/
void alarm_handler(){
  time_t curtime;//create a time variable
  if (time(&curtime) == -1) {//check the time variable
		perror("Time is error");
		exit(1);
	}
  //craete pid variable
  pid_t process_id;
  process_id = getpid();
  //print infomation
  printf("PID: %d ", process_id);
  printf("| Current Time: %s", ctime(&curtime));
  if (alarm(alarm_time) != 0) {//send alarm signal and check it
		printf("Alarm is error.\n");
		exit(1);
	}
}

/*
  This is a alarm SIGUSR Handler
*/
void SIGUSR_handler () {
  //print information
	printf("SIGUSR1 caught!\n");
	count++;
}

/*
  This is a alarm SIGINT Handler
*/
void  SIGINT_handler () {
  //print information
	printf("\nSIGINT received.\n");
	printf("SIGUSR1 was received %i times. Exiting now.\n", count);
	exit(0);
}

/* PARTIALLY COMPLETED:
 * This program is used to produce signals and handle those signals
 *
 * argc: CLA count
 * argv: CLA value
 */
int main(int argc, char *argv[]) {
  printf("Pid and time will be printed every 3 seconds.\n");
  printf("Enter ^C to end the program.\n");

  struct sigaction act;
	memset (&act, 0, sizeof(act));
  //create a sigaction for alarm signal
	act.sa_handler = alarm_handler;
  //check this sigaction
	if (sigaction(SIGALRM, &act, NULL) != 0){
		printf("Handler binding is error");
		exit(1);
	}
  //send alarm signal
	if (alarm(alarm_time) != 0) {
		printf("Alarm handler binding is error.\n");
		exit(1);
	}
  //create a sigaction for SIGUSR1 signal
  struct sigaction act2;
	memset (&act2, 0, sizeof(act2));
	act2.sa_handler = SIGUSR_handler;
  //check this sigaction
	if (sigaction (SIGUSR1, &act2, NULL) != 0) {
		perror ("Handler binding is error");
		exit(1);
	}

  struct sigaction act3;
  memset (&act3, 0, sizeof(act3));
  //create a sigaction for SIGINT signal
  act3.sa_handler = SIGINT_handler;
  //check this sigaction
  if(sigaction(SIGINT, &act3, NULL) !=0){
    printf("Handler binding is error");
    exit(1);
  }
  while (1){
  }
  return 0;
}
