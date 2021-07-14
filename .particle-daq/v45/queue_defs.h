#ifndef QUEUE_DEFS_H
# define QUEUE_DEFS_H

/*
 * queue_defs.h - sysV IPC message queue definitions & includes
 */

#ifdef REDHAT7
#include <linux/msg.h>     /* message queue functions and structs. */
#else
#include <sys/ipc.h>     /* general SysV IPC structures          */
#include <sys/msg.h>     /* message queue functions and structs. */
#endif

#define QUEUE_ID 137      /* ID of queue to generate.                         */
#define MAX_MSG_SIZE 200  /* size (in bytes) of largest message we'll send.   */
#define NUM_MESSAGES 1  /* number of messages the sender program will send. */

#define MAIN_QUEUE_ID    0x80
#define KEY_QUEUE_ID     0x81
#define TIME_QUEUE_ID    0x82
#define SERIAL_QUEUE_ID  0x83


#define TIME_TYPE   1
#define SERIAL_TYPE 2
#define KEY_TYPE    3


#define BEGIN  1   /* command to sender to begin the run */
#define END    2   /* command to sender to end run       */

#endif /* QUEUE_DEFS_H */
