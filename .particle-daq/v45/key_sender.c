/*
 * key_sender.c - a program that sends messages from the keyboard 
 *                    to the main message queue
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
  struct msgbuf* send_msg;       /* structure used for sent messages.   */
  int i;                    /* loop counter                        */
  int rc;                   /* error code retuend by system calls. */
  int msg_type;             /* contains type of message to send    */
  struct msqid_ds descr;   /* structure used for msgque ctl descrip */
  int done = 0;        /* when the program is over            */

  extern int errno;

  char dataline[100]; /* the string taken from the serial port */


 
  /* access the MAIN message queue that the reader program created. */
  main_queue_id = msgget(MAIN_QUEUE_ID, 0);
  if (main_queue_id == -1) {
    perror("Key: msgget");
    exit(1);
  }
#ifdef DEBUG
  printf("Key: MAIN message queue opened, queue id '%d'.\n", main_queue_id);
#endif


  // Allocate memory
  send_msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);


  // Form the main loop for the sender.
  // When this loop is terminated, the program is over.

  while (!done) {

    fgets(dataline,sizeof(dataline),stdin);

#ifdef DEBUG
    printf("Key: Line __LINE__, dataline is %s\n",dataline);
#endif

    send_msg->mtype = KEY_TYPE;
    sprintf(send_msg->mtext, dataline);

#ifdef DEBUG
    printf("Key: Line __LINE__\n");
#endif

    rc = msgsnd(main_queue_id, send_msg, strlen(send_msg->mtext)+1, 0);

    if (rc == -1) {
#ifdef DEBUG
      printf("Key: Line __LINE__\n");
#endif
      perror("Key: msgsnd");
      printf("errno is %i\n",errno);
      exit(1);
    }  // end if

    done = ( strstr(dataline,"q") != NULL );

  }  // end while (!done)


  /* free allocated memory. */
  free(send_msg);

#ifdef DEBUG
  printf("Key:generated %d messages, exiting.\n", NUM_MESSAGES);
#endif
  
  return 0;
}







