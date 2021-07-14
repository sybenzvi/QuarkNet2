/*
 * serial_sender.c - a program that sends messages from the serial 
 *                   port to the main message queue
 */

#include <stdio.h>       /* standard I/O functions.              */
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <sys/types.h>   /* various type definitions.            */
#include "queue_defs.h"  /* definitions shared by both programs  */
#include <unistd.h>      /* sleep(), etc.                        */
#include <asm/errno.h>   /* access to error code, e.g. EIDRM     */
#include <errno.h>
#include <string.h>
#include "configuration.h"  /* access to length of serial port, filename */

int main(int argc, char* argv[])
{
  int main_queue_id;             /* ID of the  queue for reading.  */
  struct msgbuf* send_msg;       /* structure used for sent messages.   */
  int i;                    /* loop counter                        */
  int rc;                   /* error code retuend by system calls. */
  int msg_type;             /* contains type of message to send    */
  struct msqid_ds descr;   /* structure used for msgque ctl descrip */
  int done = 0;        /* when the program is over            */

  FILE *serial;                       // Pointer to the serial port
  char serialPort[SERIAL_LENGTH];      // file id of serial port
  char newSerial[SERIAL_LENGTH];      // file id of serial port
  FILE *particle_port;
  char* token;

#define INPUTSIZE (100)
  char dataline[INPUTSIZE]; /* the string taken from the serial port */

#ifndef FAKESERIAL
  /* choice of serial port */
  sprintf(serialPort,"/dev/ttyS1"); // default
  particle_port = fopen(PORT_FILE,"r");
  if ( particle_port != NULL )
    {
      fgets(newSerial,SERIAL_LENGTH,particle_port);
      // strip everything after first space, CR or newline
      token=strtok(newSerial," \n\r"); 
      printf("NEW SERIAL: %s\n", newSerial);
      if (strcmp(newSerial,"/dev/ttyS1") && strcmp(newSerial,"/dev/ttyUSB0"))
	{
	  printf("   FATAL Error in $(HOME)/.particle-port file:\n");
	  printf("   Serial device name must be /dev/ttyUSB0 or /dev/ttyS1\n");
	  exit(-1);
	}
      else
	strcpy(serialPort,newSerial);
    }
  serial = fopen(serialPort,"r");
#endif
 
  /* access the MAIN message queue that the reader program created. */
  main_queue_id = msgget(MAIN_QUEUE_ID, 0);
  if (main_queue_id == -1) {
    perror("Serial: msgget");
    exit(1);
  }
#ifdef DEBUG
  printf("Ser: MAIN message queue opened, queue id '%d'.\n", main_queue_id);
#endif


  // Allocate memory
  send_msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);


  // Form the main loop for the sender.
  // When this loop is terminated, the program is over.

  while (!done) {

#ifdef FAKESERIAL
      sleep(10);
      sprintf(dataline,"Fake data, coming up!");
#else
      //      fscanf(serial,"%s",dataline);

#ifdef DEBUG
      /*printf("Ser: blocking on serial receive \n");*/
#endif
      fgets(dataline,INPUTSIZE,serial);
#ifdef DEBUG
      printf("Ser: received input: %s   ",dataline);
#endif
#endif

      // write to the main queue
      send_msg->mtype = SERIAL_TYPE;
      sprintf(send_msg->mtext, dataline);
      
      rc = msgsnd(main_queue_id, send_msg, strlen(send_msg->mtext)+1, 0);
    
      if (rc == -1) {
	perror("Serial: msgsnd");
	exit(1);
      }  // end if

  }  // end while (!done)


  /* free allocated memory. */
  free(send_msg);

  
#ifdef DEBUG
  printf("Ser:generated %d messages, exiting.\n", NUM_MESSAGES);
#endif
  
  return 0;
}




