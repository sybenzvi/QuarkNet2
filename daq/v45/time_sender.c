/*
 * time_sender.c - a program that sends a message
 *                   when time expires
 */

#include <stdio.h>       /* standard I/O functions.              */
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <sys/types.h>   /* various type definitions.            */

#include "queue_defs.h"  /* definitions shared by both programs  */
#include <unistd.h>      /* sleep(), etc.                        */
#include <asm/errno.h>   /* access to error code, e.g. EIDRM     */
#include <errno.h>
#include <string.h>


int main(int argc, char* argv[])
{
  int main_queue_id;             /* ID of the  queue for reading.  */
  int time_queue_id;             /* ID of the queue for sending.   */
  struct msgbuf* send_msg;       /* structure used for sent messages.   */
  struct msgbuf* rcv_msg;  /* structure for received messages     */
  int i;                    /* loop counter                        */
  int rc;                   /* error code retuend by system calls. */
  int msg_type;             /* contains type of message to send    */
  int done = 0;        /* when to stop the reader             */
  int wait = 1;        /* when to make the reader wait        */
  int start;
  struct msqid_ds descr;   /* structure used for msgque ctl descrip */
  int time;                 /* time for run                         */
  extern int errno;         /* for errors                         */

  char dataline[100];
  
  
 
  /* access the MAIN message queue that the reader program created. */
  main_queue_id = msgget(MAIN_QUEUE_ID, 0);
  if (main_queue_id == -1) {
    perror("Time:msgget");
    exit(1);
  }
#ifdef DEBUG
  printf("Time: MAIN message queue opened, queue id '%d'.\n", main_queue_id);
#endif

  // access the SERIAL message queue that the main program created.  
   time_queue_id = msgget(TIME_QUEUE_ID, 0);
   if (time_queue_id == -1) {
     perror("Serial: msgctl");
     exit(1);
   } 
#ifdef DEBUG
   printf("Time: TIME message queue opened, queue id '%d'.\n", time_queue_id);
#endif



  // Allocate memory
  rcv_msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);
  send_msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);


  // Form the main loop for the sender.
  // When this loop is terminated, the program is over.

  while (!done) {
    wait = 1;
    sleep(1);

    // Before starting, clear all messages from the time queue.
#ifdef DEBUG
    printf ("Time:I'm going to clear the queue now.\n");
#endif
    rc = msgrcv(time_queue_id, rcv_msg, MAX_MSG_SIZE+1, 0, IPC_NOWAIT);
#ifdef DEBUG
    printf("Time:First time: Errno = %i", errno);
    printf("Time:       ENOMSG = %i    \n", ENOMSG); 
#endif
    while ( (errno != ENOMSG) || ( (rc != -1) && (rcv_msg->mtype != BEGIN) ) ) {
      rc = msgrcv(time_queue_id, rcv_msg, MAX_MSG_SIZE+1, 0, IPC_NOWAIT);
#ifdef DEBUG
      printf("Time:rc = %i      ", rc);
      printf("Time:Errno = %i    ", errno); 
      printf("Time:ENOMSG = %i    ", ENOMSG); 
      printf("Time:     Removing old Q....\n");
#endif
    }

    if ( (rc != -1) && (rcv_msg->mtype != BEGIN) ) {
    } else {
      /* receive a message of any type from the TIME queue*/
#ifdef DEBUG
      printf("Time: Blocking for any message\n");
#endif
      rc = -1;
      start = 0;
      while ((rc == -1) || (!start)) {
	rc = msgrcv(time_queue_id, rcv_msg, MAX_MSG_SIZE+1, 0, 0);
#ifdef DEBUG
	printf("Time: checking time Q\n");
#endif
	if (rcv_msg->mtype == BEGIN ) {
	  start = 1;
	}
      }

    if (rcv_msg->mtype != BEGIN) {
      perror("Serial: msgrcv");
      done = 1;
      
    } else {

#ifdef DEBUG
      printf("Time: line __LINE__\n");
#endif
          
      sprintf(dataline, rcv_msg->mtext);
      time = atoi(dataline);
#ifdef DEBUG 
     printf("Time:Time is %i\n", time);
#endif

      while ( (time > 0) && (wait) ) {
	sleep(1);
	time--;
#ifdef DEBUG
	printf("Time:Sleeping......");
	printf("Time:Time is %i\n");
#endif

	rc = msgrcv(time_queue_id, rcv_msg, MAX_MSG_SIZE+1, 0, IPC_NOWAIT);
#ifdef DEBUG
	printf("Time:Errno = %i    rc = %i    ", errno, rc);
#endif
	if (rc != -1) {
	  // there was a message
	  if (rcv_msg->mtype == END) {
	    wait = 0;
#ifdef DEBUG
	    printf("Time:Received an END message\n");
#endif
	  } else {
#ifdef DEBUG
	    printf("Time:Bad message!!\n");
#endif
	  }
	} else { 	  
	  // there was no message
	  if (errno != ENOMSG) {
#ifdef DEBUG
	    printf("Time:Bad message error!!!!\n");
#endif
	  }
	}
	

      }  // end while

      // if the timer expired
      if (wait) {

	send_msg->mtype = TIME_TYPE;
	rc = msgsnd(main_queue_id, send_msg, strlen(send_msg->mtext)+1, 0);
#ifdef DEBUG
	printf("Time:Message sent to main to quit.\n");
#endif

	if (rc == -1) {
	  perror("Serial: msgsnd");
	  done = 1;
	}  

      } // end if (wait)

}    }  // end else

  } // end while (!done)


  /* free allocated memory. */
  free(send_msg);
  free(rcv_msg);
  
#ifdef DEBUG
  printf("Time: generated %d messages, exiting.\n", NUM_MESSAGES);
#endif
  
  return 0;
}  // end main




