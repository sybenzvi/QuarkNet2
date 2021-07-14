/* notes below on the Ian and Raeghan problems */

/* INTERFACE.CPP - Here's the main function for the Muon Telescope
                 Interface program (by Andrew Blechman and Wells Wulsin)

                   VERSION 1.0 (7.24.00)
*/

#include <string.h>      /* for string manipulation              */
#include <stdio.h>       /* standard I/O functions.              */
#include "quarknet.h"

#include <math.h>
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <unistd.h>      /* sleep(), etc.                        */
#include <sys/types.h>   /* various type definitions.            */
#include <asm/errno.h>   /* access to error code, e.g. EIDRM     */
#include <errno.h>
#include <sys/stat.h>    /* file status structure */
#include <unistd.h>      /* file status struction */
#include <time.h>        /* access to time_t */
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <float.h>      /* includes float limits */

#include "queue_defs.h"  /* definitions shared by both programs  */
#include "configuration.h" /* configuration */

// declare functions defined below
char* decodeMask(char* result, int code);

extern int errno;                   /* for errors from msgrcv*/

struct msgbuf* msg;       /* structure used for received messages. */

main()
{
  char choice;                        /* Menu choice*/
  unsigned char CRegister;            /* Control Register*/
  int ch1, ch2, ch3, ch4;             /* Number of muons in each channel*/
  int done;

  FILE *serial;                       /* Pointer to the serial port*/
  FILE *particle_port;                /* File spec of serial port (optional) */
  char newSerial[SERIAL_LENGTH];      /* temp storage of serial port name */

  int main_queue_id;        /* ID of the created queue.              */
  int time_queue_id;      /* ID of the created queue.              */
  int key_queue_id;         /* ID of the created queue.              */
  int serial_queue_id;        /* ID of the created queue.              */
  struct msqid_ds descr;   /* structure used for msgque ctl descrip */

  int rc;                   /* error code returned by system calls.  */

  int msg_type;             /* type of messages we want to receive.  */
  int i;                    /* loop counter                          */

  unsigned long timestamp, fakestamp, code;
  unsigned long eventcounter;         /* number of events       */
  unsigned long nInterval;
  double totaltime;     /* the total cumulative time in the run */
  double difftime;
  double etime;         /* time of electron hit in muon decay */
  double detime;        /* decay time of muon*/
  int rateInterval;  /* the number of the sub-run intervals over
                        which the rates are logged */
  int previousRateInterval;

  // version1: time interval, 32MHz clock/($3FFB+1) = 8 microsec
  // version2: time interval is 1/6MHz
  double interval;
  //for muon clock rate is different 32MHz/ww1
  double muinterval;

  float theRate, rateSigma, flInterval;

  int badchar=0;
  int gotData=0;

  char timestr[100];
  char message[20];
  char sscalars[100];
  char read[5];
  char dataline[100];
  int timesize=21;
  char timestampstr[timesize];
  char codestr[100];

  char coderep[12];
  char mucoderep[12];

  char datasend[100];
  char datasend2[100];
  const int lengthFormattedDate = 20;
  char formattedDate[lengthFormattedDate];
  char test[5];
  char filestr[120];

  /*added file names for down,up, and noise muons*/
  char downstr[120];
  char noisestr[120];
  char upstr[120];

  /*add arrays for muon histograms*/
  const int coarseWidth = 10;
  int histBin,coarseBin;
  const int maxBin = 500;
  const int maxCoarse = maxBin/coarseWidth;
  int downHist[maxBin],upHist[maxBin],noiseHist[maxBin];
  int downCoarse[maxCoarse],upCoarse[maxCoarse],noiseCoarse[maxCoarse];

  /*new board clock frequency in Hz*/
  const double clockFreq = 41666369.;
  const double clockRollOver = ((double) pow(2,32))/clockFreq;

  /*when to declare a second hit is a muon decay? (in microseconds)*/
  const double decayThresh = 0.2 ;  // n.b., FBT code used 0.1 microseconds
  double gateAwareDecayThresh;

  /* Add array for efficiency sort */
  int efficiency[16];
  int previousEfficiency[16];

  char filename[90];
  char suffix[90];
  char ch1str[20];
  char ch2str[20];
  char ch3str[20];
  char ch4str[20];

  int veto,channelMask,coinLevel;

  char* token;

  interfaceConfiguration* theConfiguration;

  char choicechar;      // used in menu operation

  done = 0;             /* when to stop the reader               */
  sprintf(filename,"timestamp"); /* default output file */

  msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);

  /* establish a default configuration */

  theConfiguration =
    (interfaceConfiguration*) malloc(sizeof(interfaceConfiguration));
  theConfiguration->reportSats = FALSE;
  theConfiguration->timeFix = FALSE;
  theConfiguration->beep = FALSE;
  theConfiguration->debug = FALSE;
  theConfiguration->rawoutputfile = FALSE;
  theConfiguration->outputfile = TRUE;
  theConfiguration->useGPS = FALSE;
  theConfiguration->rawGPSdata = FALSE;
  theConfiguration->useAvgFreq = FALSE;
  theConfiguration->avgAmount = 4;
  theConfiguration->writeEachHit = TRUE;
  theConfiguration->rateSummary = 0;
  theConfiguration->muon = FALSE;
  theConfiguration->efficient = TRUE;
  theConfiguration->hexCode = FALSE;
  theConfiguration->version = 0;       // old board default
  theConfiguration->runlen = 10;       // singles run time default
  theConfiguration->barA = 0.0;        // barometer calibration
  theConfiguration->barB = 22.0;       // defaults, seem to work ~OK
  theConfiguration->barometerCalibrated = FALSE; // not calibrated by default
  theConfiguration->rateSummaryGoodies = TRUE;  // temp and pressure
  theConfiguration->muonSpeed = FALSE; // muon speed measurement
  theConfiguration->riseFallTimes = FALSE; // rising and falling time printout
  theConfiguration->triggerWidth = 0; // extra trigger width
  theConfiguration->rawBoard = FALSE;
  theConfiguration->absorber = 0;      // absorber not present by default

  /* choice of serial port */
  sprintf(theConfiguration->serialDevice,"/dev/ttyS1"); // default
  particle_port = fopen(PORT_FILE,"r");
  //printf("opening %s...",PORT_FILE);
  if ( particle_port != NULL )
    {
      if (theConfiguration->debug) printf(" success\n");
      fgets(newSerial,SERIAL_LENGTH,particle_port);
      fclose(particle_port);
      // strip everything after first space, CR or newline
      token=strtok(newSerial," \n\r");
      if (theConfiguration->debug)
          printf("stripped newSerial = %s\n",newSerial);
      if (strcmp(newSerial,"/dev/ttyS1") && strcmp(newSerial,"/dev/ttyS0"))
	{
	  printf("   FATAL Error in $(HOME)/.particle-port file:\n");
	  printf("   Serial device name must be /dev/ttyS0 or /dev/ttyS1\n");
	  exit(-1);
	}
      else
	strcpy(theConfiguration->serialDevice,newSerial);
    }

  CRegister=0x0f;                /* all channels on, singles, no veto */
  ch1 = 0;                            /* Clear the channels */
  ch2 = 0;
  ch3 = 0;
  ch4 = 0;

  for(i = 1;i < 16; i++){
    efficiency[i]=0;
    previousEfficiency[i]=0;
  }

  for(i = 0;i < maxBin; i++){
    upHist[i]=0;downHist[i]=0;noiseHist[i]=0;
  }

  for(i = 0;i < maxCoarse; i++){
    upCoarse[i]=0;downCoarse[i]=0;noiseCoarse[i]=0;
  }

  serial=fopen(theConfiguration->serialDevice,"a");


  /* Create message queues.*/

  /* Setup the MAIN message queue.*/

  /* check for an existing message queue with the desired keu */
  /*  if found, blow it away */

  main_queue_id = msgget(MAIN_QUEUE_ID, 0);
  if (main_queue_id != -1) {
    if ( msgctl(main_queue_id, IPC_RMID, &descr) != 0 ) {
      perror("main: msgctl");
      done = 1;
    }
  }

  /* create a public message queue, with access only to the owning user. */
  main_queue_id = msgget(MAIN_QUEUE_ID, IPC_CREAT | IPC_EXCL | 0600);
  if (main_queue_id == -1) {
    perror("main: msgget");
    done = 1;
  }
  if (theConfiguration->debug)
    printf("Main:MAIN message queue created, queue id '%d'.\n", main_queue_id);


  /* Setup the TIME message queue.*/

  /* check for an existing TIME message queue with the desired keu */
  /*  if found, blow it away */

  time_queue_id = msgget(TIME_QUEUE_ID, 0);
  if (time_queue_id != -1) {
    if ( msgctl(time_queue_id, IPC_RMID, &descr) != 0 ) {
      perror("Time: msgctl");
    }
  }

  /* create a TIME message queue, with access only to the owning user. */
  time_queue_id = msgget(TIME_QUEUE_ID, IPC_CREAT | IPC_EXCL | 0600);
  if (time_queue_id == -1) {
    perror("Time: msgget");
    printf("Expected error making time queue.\n");
  }
  if (theConfiguration->debug)
    printf("Main:TIME message queue created, queue id '%d'.\n", time_queue_id);


  /*                                                          */
  /* attempt to automatically guess the version of the board  */
  /*                                                          */

  sleep(1);           // wait for other jobs to start...
  if (strcmp(theConfiguration->serialDevice,"/dev/ttyS1") == 0)
    system("stty -F /dev/ttyS1 ispeed 9600 ospeed 9600");
  else
    system("stty -F /dev/ttyS0 ispeed 9600 ospeed 9600");

  usleep(20000);

  /* empty the serial queue */
  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) ) {
    rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
    if (theConfiguration->debug){
      sprintf(dataline, msg->mtext);
      printf("Main: removing from serial %s\n",dataline);
    }
  }
  serialSend(serial,"ID\r");
  /* wait 1 second for response, then read messages on queue */
  usleep(50000);
  rc = 0;
  gotData = 0;

  rc = msgrcv(main_queue_id, msg,
              MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
  while ( (errno != ENOMSG) || (rc != -1) ) {
    sprintf(dataline, msg->mtext);
    if ( strstr(dataline,"Assembly") != NULL )
    {
      theConfiguration->version = 1;
      printf("Revised version of old board detected.\n");
    }
    rc = msgrcv(main_queue_id, msg,
                MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
    gotData = 1;
  }
  if (theConfiguration->version != 1)
  {
    if (strcmp(theConfiguration->serialDevice,"/dev/ttyS1") == 0)
      system("stty -F /dev/ttyS1 ispeed 19200 ospeed 19200");
    else
      system("stty -F /dev/ttyS0 ispeed 19200 ospeed 19200");
    usleep(20000);
    serialSend(serial,"CD\r");
    serialSend(serial,"ID\r");
    rc = 0;
    rc = msgrcv(main_queue_id, msg,
                MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
    while ( (errno != ENOMSG) || (rc != -1) )
    {
      gotData = 1;
      sprintf(dataline, msg->mtext);
      if (strstr(dataline,"Command") != NULL)
      {
        printf("New board detected.\n");
        theConfiguration->version = 2;
        serialSend(serial,"WC 0 1F\r");
      }
      rc = msgrcv(main_queue_id, msg,
                  MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
    }
  }
  if (!gotData)
  {
    printf("Error: Could not communicate with board! Check connection.\n");
    exit(-1);
  }
  if (theConfiguration->version == 0)
  {
    printf("Using default setting: old board.\n");
    if (strcmp(theConfiguration->serialDevice,"/dev/ttyS1") == 0)
      system("stty -F /dev/ttyS1 ispeed 9600 ospeed 9600");
    else
      system("stty -F /dev/ttyS0 ispeed 9600 ospeed 9600");
    usleep(20000);
  }

  do
    {

      /* Write out the main menu*/
      usleep(250000);  /* sleep for 1/4 sec */
      choicechar = mainMenu(filename,theConfiguration);

      switch(choicechar)
        {
        case '1':   /* Write to the control register */
         {
          char msg[10];

          CRContents(CRegister);
          CRegister = CRWriteMenu(CRegister);
          printf("The encoded hex number is %x\n",CRegister);
          if (theConfiguration->version == 2)
          {
            sprintf(msg,"WC 0 %x\r",CRegister);
            serialSend(serial, msg);
          }
          break;
         }
          /**************************************/

        case '2':   /* Display contents of the control register */

          if (theConfiguration->version == 2)
            CRegister = getCRStatus(serial, main_queue_id);
          CRContents(CRegister);
          break;

          /**************************************/

        case '3':   // single counter rates run
	  if (theConfiguration->version != 2)
          {
            char a, strin[20];
            int secs,j, runlen;
            badchar = 0;

            serialSend(serial,"NE\r");
            serialSend(serial,"WC 0\r");
            serialSend(serial,"RS\r");

            printf("Starting a %i second run to calculate rates...\n",theConfiguration->runlen);
            serialSend(serial,"WI 3 1\r");
            serialSend(serial,"WW 1\r");
            serialSend(serial,"ES\r");
            if (theConfiguration->version == 0){
              serialSend(serial,"WD 3FFB FF\r");
              interval = .000008; /* hard-wire interval, WD 3FFB FF */
            }
            else {
              serialSend(serial,"TP 0\r");
              interval = 1.e0/6.e+6; /* 1/(6MHz) clock rate */
              serialSend(serial,"SS\r");
            }
            serialSend(serial,"WC F\r");

            if (theConfiguration->debug)
              printf("Main: sent: %s\n",message);

            for (secs=1; secs<=theConfiguration->runlen; secs++)
              {
                sleep(1);
                printf("."); fflush(stdout);
              }
            printf("\n");

            serialSend(serial,"WC 0\r");

            if (theConfiguration->debug)
              printf("Main: sent: %s\n",message);

            /* now sleep briefly and then empty the serial queue */
            usleep(100000);
            rc = 0;
            while ( (errno != ENOMSG) || (rc != -1) ) {
              rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
              if (theConfiguration->debug){
                sprintf(dataline, msg->mtext);
                printf("Main: removing from serial %s\n",dataline);
              }
            }

            sprintf(message,"DS\r");
            fprintf(serial,message);
            fflush(serial); usleep(20000); // 20 characters at 9600 baud
            if (theConfiguration->debug)
              printf("Main: sent: %s\n",message);

            /* wait 1 second for response, then read messages on queue */
            sleep(1);
            rc = 0;
            gotData = 0;

            while ( (errno != ENOMSG) || (rc != -1) ) {
              rc = msgrcv(main_queue_id, msg,
                          MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);

              sprintf(dataline, msg->mtext);

              /* test to make sure the line is valid */
              badchar = 0;

              /* proper output must have at least 35 characters */
              if (strlen(dataline) < 35) {
                badchar = 1;
                if (theConfiguration->debug)
                  printf ("Line too short; string omitted.\n");
              }

              if (!badchar) {
                i = 0;
                /* clear white space at the beginning (includes prompt) */
                while (dataline[i] == ' ' || dataline[i]=='>') {
                  i++;
                }

                while (!( (i+1) >= strlen(dataline) ) ) {
                  test[0] = dataline[i++];
                  test[1] = (char) NULL;
                  /* check to see if character is an acceptable hex */
                  if (strstr("0123456789abcdefABCDEF ", test) == NULL) {
                    badchar = 1;
                    if (theConfiguration->debug)
                      printf("Badchar %s found at character %i\n",test,i);
                  }
                }

                i=0;
                if (!badchar) {
                  printf("Scaler data received:%s",dataline);
                  j=0;

                  while (!( (dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) ) ) {
                    ch1str[j++] = dataline[i];
                    i++;
                  }

                  /* terminate ch1str */
                  ch1str[j] = (char) NULL;

                  // clear white space in the middle
                  while (dataline[i] == ' ') {
                    i++;
                  }

                  j=0;
                  while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) ) ) {
                    ch2str[j++] = dataline[i];
                    i++;
                  }

                  /* terminate ch2str */
                  ch2str[j] = (char) NULL;

                  // clear white space in the middle
                  while (dataline[i] == ' ') {
                    i++;
                  }

                  j=0;
                  while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) ) ) {
                    ch3str[j++] = dataline[i];
                    i++;
                  }

                  /* terminate ch3str */
                  ch3str[j] = (char) NULL;

                  // clear white space in the middle
                  while (dataline[i] == ' ') {
                    i++;
                  }

                  j=0;
                  while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) ) ) {
                    ch4str[j++] = dataline[i];
                    i++;
                  }

                  /* terminate ch4str */
                  ch4str[j] = (char) NULL;


                } // if (!badchar)

                // Convert strings to numbers.
                if (theConfiguration->version == 1)
                  {
                    // order is backwards... fix here
                    sscanf(ch4str, "%xl", &ch1);
                    sscanf(ch3str, "%xl", &ch2);
                    sscanf(ch2str, "%xl", &ch3);
                    sscanf(ch1str, "%xl", &ch4);
                  }
                else
                  {
                    sscanf(ch1str, "%xl", &ch1);
                    sscanf(ch2str, "%xl", &ch2);
                    sscanf(ch3str, "%xl", &ch3);
                    sscanf(ch4str, "%xl", &ch4);
                  }
                // print out scalars
                printf("Channel 1 recorded %i hits.\n", ch1);
                printf("Channel 2 recorded %i hits.\n", ch2);
                printf("Channel 3 recorded %i hits.\n", ch3);
                printf("Channel 4 recorded %i hits.\n", ch4);

                gotData = 1;

              } //if (!badchar)

              if (theConfiguration->debug)
                printf("Main: received from serial %s\n",dataline); /* echo to screen */
            }  // end while loop

            if (!gotData) {
              printf("The board is not responding.  Push the reset button and repeat test.\n");
            }

            /* a = receiveKeyChar(); wait for user */

            if (theConfiguration->debug) {
              printf("Main:rc = %i      ", rc);
              printf("Main:Errno = %i    ", errno);
              printf("Main:ENOMSG = %i    ", ENOMSG);
              printf("Main:     Removing old Q....\n");
            }

          }
          else		// we've got a new board
          {
            int secs, cha1, cha2, cha3, cha4, triggers;
            int chb1, chb2, chb3, chb4, triggersb;
            char msgdata[12];

            serialSend(serial,"CD\r");   // stop the FIFO output
            usleep(100000);
            printf("\nData will be accumulated for %i seconds...",
              theConfiguration->runlen);
            fflush(stdout);
            getCounts(&cha1, &cha2, &cha3, &cha4, &triggers, serial,
              main_queue_id);
            for (secs = 0; secs < theConfiguration->runlen; secs++)
            {
               sleep(1);
               printf(".");
               fflush(stdout);
            }
            printf("\nFinished collecting data.  Calculating rates...\n");
            getCounts(&chb1, &chb2, &chb3, &chb4, &triggersb, serial,
              main_queue_id);
	    //serialSend(serial,"CE\r");   // reenable the FIFO output
            printf("Channel one counted %i hits.\n",chb1 - cha1);
            printf("Channel two counted %i hits.\n",chb2 - cha2);
            printf("Channel three counted %i hits.\n",chb3 - cha3);
            printf("Channel four counted %i hits.\n",chb4 - cha4);
            if (theConfiguration->debug)
              printf("DEBUG: Counted %i triggers.\n",triggersb - triggers);
            sleep(1);
          }
          break;

        case '4':
        case '5':   // Begin Run
          // Outline of processes for begin run.
          // 1a: tell time sender to begin run
          // 1b: tell board to begin run
          // 2: read from main Q
          // 3:  tell board to end run
          if (theConfiguration->version == 2)
          {
            unsigned char oldCR, evtSingle, evtSingleNew, evtMultipleNew;
            char hitTimeStr[10], test[2], timeA[3], timeB[3], output[15], rawfilestr[124],
                 uoutput[15], edgeOutput[100], edgeOutputFile[100], edgeTemp[100];
            int j, k, eventNum, timeAVal, timeBVal, prexit, first, GPSvalid, newGPSvalid,
                interval, intervalEventNum, updateInterval, newGPShour, newGPSminute,
		GPShour, GPSminute, intervalEventSum, insideEvent, first2, first3, hipad,
	      lapped, hourOffset, numSats, DATAvalid, newDATAvalid;
            int evtList[16], intervalEvtList[16], intervalSumList[16],
                muonDecays[4], muonHist[100], uHistIndex, printedSomething,
                decayHist[100], absHist[100], upHist[100], downHist[100];
            double speedT1, speedT2, speedT1old, speedT2old, muonDeltaT[4], GPSsecond, newGPSsecond,
                   speedT1f, speedT2f, speedT1fold, speedT2fold, lastDenom, avgDenom;
            unsigned long int eventTimeRaw, lastPPS;
            double eventTime, prevTime, runTime, rollTime, timeFix, gotAMuon;
	    double wallEventTime, prevWallEventTime;
            double t1[4], t2[4];
	    double t1fine[4], oldt1fine[4], disptime[4];
	    double t2fine[4], oldt2fine[4], oldMintimeFine;
	    double someVar, firstHitTime, aHitTime;
            time_t theTime, intervalTime;
            struct timeval preciseTime;
            struct timezone tzone;
            FILE *outfile, *decayfile, *absfile, *upfile, *downfile, *rawoutfile;

	    // set GPS avg freq
	    int frqCnt, frqEnd;
	    if (theConfiguration->useGPS) {
	      if (theConfiguration->useAvgFreq)
		frqEnd = theConfiguration->avgAmount;
	      else frqEnd = 4;
	      for (frqCnt = 0, avgDenom = 0; frqCnt < frqEnd; frqCnt++)
		avgDenom += (double) getGPSfrq(serial, main_queue_id);
	      avgDenom /= (double) frqEnd;
	    }

	    // reset TMC and CPLD... but would this be safe?
	    serialSend(serial, "RB\r");
	    usleep(50000);

	    serialSend(serial, "CD\r");

	    printf("How long would you like the run to be in seconds?\n");
            receiveKeyString(timestr);


if (1) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


  	    if (!theConfiguration->useGPS) serialSend(serial, "WC 1 6E\r");


            if (theConfiguration->muon)
            {
              serialSend(serial, "WC 2 41\r"); // set 20 us gate
              serialSend(serial, "WC 3 3\r");
	      if (theConfiguration->triggerWidth != 0) {
		// set a one microsecond width (42 clock ticks)
		serialSend(serial, "WT 01 0\r");
		serialSend(serial, "WT 02 2A\r");
		gateAwareDecayThresh = 1.+decayThresh;
	      }
	      else {
		// set default trigger width (6 clock ticks, .14 us)
		serialSend(serial, "WT 01 0\r");
		serialSend(serial, "WT 02 6\r");
		gateAwareDecayThresh = decayThresh;
	      }
            }
            else
            {
	      if (theConfiguration->triggerWidth != 0) {
		// set a one microsecond width (42 clock ticks)
		serialSend(serial, "WT 01 00\r");
		serialSend(serial, "WT 02 2A\r");
		serialSend(serial, "WC 2 54\r");
		serialSend(serial, "WC 3 0\r");
		gateAwareDecayThresh = 1.+decayThresh; // or set to infinity?
	      }
	      else {
		serialSend(serial, "WT 01 00\r");
		serialSend(serial, "WT 02 06\r"); // set (default) .14us trig width
		serialSend(serial, "WC 2 0C\r"); // set (default) .24 us gate
		serialSend(serial, "WC 3 0\r");
		gateAwareDecayThresh = decayThresh; // or set to infinity?
	      }
           }

            if (choicechar == '4')
            {
              oldCR = CRegister;         // save current CR
              CRegister = 0x1f;
              serialSend(serial, "WC 0 1F\r");
            }
            else
            {
              sprintf(message, "WC 0 %x\r", CRegister);
              serialSend(serial, message); // in case they reset
            }
}//!!!!!!!!!!!!!!!!!!!!

            // force timer to start counting (i.e. tell it not to wait
            // for 1PPS GPS pulse)
            serialSend(serial, "CE\r");

            usleep(50000);

            theTime = time(0);
            strftime(timestampstr,timesize,"%m-%d-%y--%H-%M-%S",
                     localtime(&theTime));
            if (strcmp(filename,"timestamp") == 0)
              sprintf(suffix,timestampstr);
            else
              sprintf(suffix,filename);
            sprintf(filestr,"%s/muon.%s",getenv("HOME"),suffix);

	    if (theConfiguration->rawoutputfile) {
	      sprintf(rawfilestr, "%s/raw.%s", getenv("HOME"), suffix);
	      printf("Opening raw output file %s\n", rawfilestr);
	      if ( (rawoutfile = fopen(rawfilestr,"w")) == NULL) {
	        printf(" Failed to open raw file %s,error:%s\n",rawfilestr,strerror(errno));
  		break;
	      }
	      else fprintf(rawoutfile, "Raw Data Output\nbegin timestamp: %s\n\n", timestampstr);
	    }
            if (theConfiguration->outputfile)
            {
              printf("Opening output file %s\n",filestr);
              if ( (outfile = fopen(filestr,"w")) == NULL)
              {
                //printf(" Failed to open file %s,error:%s\n",filestr,sys_errlist[errno]);
                printf(" Failed to open file %s,error:%s\n",filestr,strerror(errno));
		break;
	      }
              if (theConfiguration->muon)
              {
                sprintf(filestr,"%s/decay.%s",getenv("HOME"),suffix);
                printf("Opening decay file %s\n",filestr);
                if ( (decayfile = fopen(filestr,"w")) == NULL)
                {
                  //printf(" Failed to open file %s,error:%s\n",filestr,sys_errlist[errno]);
                  printf(" Failed to open file %s,error:%s\n",filestr,strerror(errno));
                  fclose(outfile);
                  break;
                }
              }
              if (theConfiguration->muon > 1)
              {
                sprintf(filestr,"%s/absorber.%s",getenv("HOME"),suffix);
                printf("Opening absorber file %s\n",filestr);
                if ( (absfile = fopen(filestr,"w")) == NULL)
                {
		  //printf(" Failed to open file %s,error:%s\n",filestr,sys_errlist[errno]);
                  printf(" Failed to open file %s,error:%s\n",filestr,strerror(errno));
                  fclose(decayfile);
                  fclose(outfile);
                  break;
	        }
                sprintf(filestr,"%s/up.%s",getenv("HOME"),suffix);
                printf("Opening up file %s\n",filestr);
                if ( (upfile = fopen(filestr,"w")) == NULL)
                {
                  //printf(" Failed to open file %s,error:%s\n",filestr,sys_errlist[errno]);
                  printf(" Failed to open file %s,error:%s\n",filestr,strerror(errno));
                  fclose(absfile);
                  fclose(decayfile);
                  fclose(outfile);
                  break;
	        }
                sprintf(filestr,"%s/down.%s",getenv("HOME"),suffix);
                printf("Opening down file %s\n",filestr);
                if ( (downfile = fopen(filestr,"w")) == NULL)
                {
                  //printf(" Failed to open file %s,error:%s\n",filestr,sys_errlist[errno]);
                  printf(" Failed to open file %s,error:%s\n",filestr,strerror(errno));
                  fclose(upfile);
                  fclose(absfile);
                  fclose(decayfile);
                  fclose(outfile);
                  break;
	        }
              }
            }


            // start the timer
            msg->mtype = BEGIN;
            sprintf(msg->mtext, timestr);
            rc = msgsnd(time_queue_id, msg, strlen(msg->mtext)+1, 0);

            // clear the main queue
            rc = 0;
            while ((errno != ENOMSG) || (rc != -1))
              rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, 0, IPC_NOWAIT);

            printf("Run active - enter (6) to abort\n");
            if (theConfiguration->outputfile)
            {
              fprintf(outfile,"Run started on %s\n",ctime(&theTime));
              if (theConfiguration->muonSpeed > 0)
              {
		if ((theConfiguration->muonSpeed&512) != 0) {  // run time selection
		  int SpFirst = 0;
		  int SpSecond = 0;
		  int indata;
		  char strin[20];
		  printf("You asked to enter the paddles for muon speed mode at run time.  \n");
		  while (SpFirst < 1 || SpFirst > 4) {
		    printf("Choose the first paddle (enter channel number  1, 2, 3, or 4):\n");
		    receiveKeyString(strin);
		    sscanf(strin, "%i", &indata);
		    if (indata < 1 || indata > 4)
		      {
			printf("Invalid choice for channel.  Must be 1, 2, 3 or 4\n");
		      }
		    SpFirst = indata;
		  }
		  while (SpSecond < 1 || SpSecond > 4) {
		    printf("Choose the second paddle (enter channel number  1, 2, 3, or 4):\n");
		    receiveKeyString(strin);
		    sscanf(strin, "%i", &indata);
		    if (indata < 1 || indata > 4)
		      {
			printf("Invalid choice for channel.  Must be 1, 2, 3 or 4\n");
		      }
		    SpSecond = indata;
		  }
		  theConfiguration->muonSpeed = (1<<SpFirst)+(16<<SpSecond)+512;
		}
		fprintf(outfile, "Muon Speed Mode on: ");
		if (theConfiguration->muonSpeed > 1)
		  {
		    fprintf(outfile,"from channel ");
		    switch ((theConfiguration->muonSpeed&30)>>1)
		      {
		      case 1: fprintf(outfile,"1 "); break;
		      case 2: fprintf(outfile,"2 "); break;
		      case 4: fprintf(outfile,"3 "); break;
		      case 8: fprintf(outfile,"4 "); break;
		      }
		    fprintf(outfile,"to  ");
		    switch ((theConfiguration->muonSpeed&480)>>5)
		      {
		      case 1: fprintf(outfile,"1"); break;
		      case 2: fprintf(outfile,"2"); break;
		      case 4: fprintf(outfile,"3"); break;
		      case 8: fprintf(outfile,"4"); break;
		      }
		    fprintf(outfile,"\n");
		  }
		else fprintf(outfile,"any two channels)\n");
	      }
	      if (theConfiguration->useGPS) readGPS(serial, main_queue_id, outfile);
              if (theConfiguration->rateSummary > 0)
              {
                fprintf(outfile, "Interval\tEnd Date\tEnd Time\tCounts\tRate(Hz)\tSigma Rate (Hz)");
                if (theConfiguration->efficient)
                  for (j = 1; j < 16; j++)
                  {
                    sprintf(output, "(");
                    if (j & 1) strcat(output, "1,");
                    else strcat(output, "-,");
                    if (j & 2) strcat(output, "2,");
                    else strcat(output, "-,");
                    if (j & 4) strcat(output, "3,");
                    else strcat(output, "-,");
                    if (j & 8) strcat(output, "4)");
                    else strcat(output, "-)");
                    fprintf(outfile, "\t%s", output);
                  }
                if (theConfiguration->rateSummaryGoodies)
                  fprintf(outfile, "\tPressure(kPa)\tTemp(degC)");
                fprintf(outfile, "\n");
              }
            }
            done = 0;
            eventNum = 0;
            intervalEventNum = 0;
            intervalEventSum = 0;
            interval = 0;
            updateInterval = 0;
            runTime = 0.0;
            eventTime = 0.0;
            insideEvent = 0;
            evtSingleNew = 0;
            timeFix = 0.0;
            speedT1 = 0;
            speedT2 = 0;
            speedT1f = 0;
            speedT2f = 0;
            hipad = 0;
            first = 1;       // first event coming up
            first2 = 1;
            first3 = 1;
            for (j = 0; j < 16; j++) evtList[j] = 0;
            for (j = 0; j < 16; j++) intervalEvtList[j] = 0;
            for (j = 0; j < 16; j++) intervalSumList[j] = 0;
            for (j = 0; j < 4; j++) muonDecays[j] = 0;
            for (j = 0; j < 4; j++) muonDeltaT[j] = 0;
            for (j = 0; j < 100; j++) muonHist[j] = 0;
            for (j = 0; j < 100; j++) decayHist[j] = 0;
            for (j = 0; j < 100; j++) absHist[j] = 0;
            for (j = 0; j < 100; j++) upHist[j] = 0;
            for (j = 0; j < 100; j++) downHist[j] = 0;
	    lapped = 0; // for GPS only
	    GPSvalid = 1; // for GPS only
	    DATAvalid = 1; // for GPS only
	    newGPSvalid = 1; // for GPS only
	    newDATAvalid = 1; // for GPS only
	    newGPSsecond = 0.;
	    newGPSminute = 0;
	    newGPShour = 0;
            while (!done)
            {
              rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, 0, 0);
              sprintf(dataline, msg->mtext);
              switch (msg->mtype)
              {
                case KEY_TYPE:
                  printf("%s", dataline);
                  if (dataline[0] == '6')
                  {
                    printf("Aborting the run...\n");
                    theTime = time(0);
                    if (theConfiguration->outputfile)
                      fprintf(outfile, "\n-- Run aborted --\n");
                    done = 2;       // bailing out early
                  }
                  else printf("Invalid command.  Enter (6) to stop run.\n");
                  break;
                case TIME_TYPE:
                  done = 1;
                  theTime = time(0);  // record stop time
                  break;
                case SERIAL_TYPE:
                  if (theConfiguration->rawBoard) {
                    printf("%s\n" , dataline);
		  }
		  if (theConfiguration->rawoutputfile)
		    fprintf(rawoutfile, "%s\n", dataline);
		  for (i = 0; dataline[i] == ' ' || dataline[i] == '>'; i++);
                  for (j = 0; j < 8; j++) hitTimeStr[j] = dataline[i++];
                  hitTimeStr[j] = (char)NULL;

                  // check for valid event
                  prexit = 0;
                  for (j = 0; j < 8; j++)
                  {
                    test[0] = hitTimeStr[j];
                    test[1] = (char)NULL;
                    if (strstr("0123456789abcdefABCDEF", test) == NULL) prexit = 1;
                  }
                  if (prexit) break;
                  sscanf(hitTimeStr, "%xl", &eventTimeRaw);

                  i++;                      // look at RE and FE
                  timeA[0] = dataline[i++]; // times to determine
                  timeA[1] = dataline[i++]; // if an event took place
                  timeA[2] = (char)NULL;
                  i++;
                  timeB[0] = dataline[i++];
                  timeB[1] = dataline[i++];
                  timeB[2] = (char)NULL;
                  sscanf(timeA, "%xl", &timeAVal);
                  sscanf(timeB, "%xl", &timeBVal);
                  if (timeAVal & 128)   // start of new event
                  {
                    insideEvent = 1;
                    evtSingle = evtSingleNew;
                    evtSingleNew = 0;  // reset event data
		    evtMultipleNew = 0; // and clear previous multiple hits
                    if (first3) first3 = 0;
                    else first2 = 0;
                    speedT1old = speedT1;
                    speedT2old = speedT2;
                    speedT1 = 0;
                    speedT2 = 0;
                    speedT1fold = speedT1f;
                    speedT2fold = speedT2f;
                    speedT1f = 0;
                    speedT2f = 0;
                    hipad = 0;
                    timeAVal -= 128;
		    // save fine times for output; subtract off minimum
		    oldMintimeFine = DBL_MAX;
		    if (theConfiguration->debug) {
		      printf("DEBUG: raw fine rising edges=%f,%f,%f,%f\n",t1fine[0],t1fine[1],t1fine[2],t1fine[3]);
		      printf("DEBUG: raw fine falling edges=%f,%f,%f,%f\n",t2fine[0],t2fine[1],t2fine[2],t2fine[3]);
		    }
		    for (k = 0; k < 4; k++) {
		      oldt1fine[k]=t1fine[k];
		      oldt2fine[k]=t2fine[k];
		      if (t1fine[k]>0. && t1fine[k]<oldMintimeFine )
			oldMintimeFine = t1fine[k];
		      if (t2fine[k]>0. && t2fine[k]<oldMintimeFine )
			oldMintimeFine = t2fine[k];
		      t1fine[k]=0.;
		      t2fine[k]=0.;
		    }
		    for (k = 0; k < 4; k++) {
		      if (oldt1fine[k]>0.) oldt1fine[k] -= oldMintimeFine;
		      if (oldt2fine[k]>0.) oldt2fine[k] -= oldMintimeFine;
		    }
		    if (theConfiguration->debug) {
		      printf("DEBUG: corrected rising edges=%f,%f,%f,%f\n",oldt1fine[0],oldt1fine[1],oldt1fine[2],oldt1fine[3]);
		      printf("DEBUG: corrected falling edges=%f,%f,%f,%f\n",oldt2fine[0],oldt2fine[1],oldt2fine[2],oldt2fine[3]);
		    }
		  }
                  else    // already in an event, must get more hits
                  {
                    insideEvent = 0;
                  }
                  if (timeAVal & 32) // rising edge found!
		    {
		      aHitTime = ((double)eventTimeRaw) / 41.666369;
		      if (t1fine[0] == 0.)  { // have not recorded a hit yet
			t1fine[0] = aHitTime*1000. + (timeAVal-32)*0.75;  // precise time in ns
		      }
		      // check to see if ANY counter has been hit in the event
		      // AND if this hit is far enough after first hit to be a muon decay
		      if ( (evtSingleNew != 0) && ( (aHitTime-firstHitTime) > gateAwareDecayThresh ) )   // muon decay
			{
			  if (muonDeltaT[0] == 0) // first muon decay hit in this paddle?
			    {
			      t2[0] = aHitTime;
			      muonDeltaT[0] = aHitTime - firstHitTime;  // use global event time as the baseline
			      if (theConfiguration->debug)
				fprintf(outfile, "DEBUG: Tagged as muon decay, paddle %i\n" , 0+1);
			      if (evtMultipleNew == 0) // if this is the first "muondecay" hit of the event
				{
				  evtMultipleNew |= 1 ; // set multiple hit bits
				  uHistIndex = (int)(muonDeltaT[0] * 5);
				  if (uHistIndex > 99) uHistIndex = 99;
				  if (uHistIndex < 0) uHistIndex = 0;
				  muonHist[uHistIndex]++;
				  muonDecays[0]++;
				  if (hipad == 1)
				    {
				      decayHist[uHistIndex]++;
				      if (hipad == theConfiguration->muon - 1)
					{
					  absHist[uHistIndex]++;
					  if (theConfiguration->muon == 2)
					    upHist[uHistIndex]++;
					  else if (theConfiguration->muon == 3)
					    downHist[uHistIndex]++;
					}
				    } // hipad?
				} // first muon decay hit anywhere... histogram
			    } // first muondecay for this paddle?
			} // muondecay hit
		      else  // it is a new event
			{
			  t1[0] = aHitTime;
			  if (evtSingleNew == 0) firstHitTime = t1[0]; // set the global first time of the event
			  evtSingleNew |= 1; // set bit for channel 1
			  if (theConfiguration->muonSpeed == 1)
			    {
			      if (speedT1 == 0.0)
				{
				  if (speedT2 == 0.0)
				    speedT2 = t1[0]*1000 + (timeAVal-32)*0.75;
				  else
				    speedT1 = t1[0]*1000 + (timeAVal-32)*0.75;
				}
			    }
			  else if (theConfiguration->muonSpeed > 1)
			    {
			      if ((theConfiguration->muonSpeed&30) == 2)
				speedT2 = t1[0]*1000 + (timeAVal-32)*0.75;
			      if ((theConfiguration->muonSpeed&480) == 32)
				speedT1 = t1[0]*1000 + (timeAVal-32)*0.75;
			    }
			  if (hipad < 1) hipad = 1;
			} // new event
		    } // rising edge
		  if (timeBVal & 32) { // falling edge
		    someVar = ((double)eventTimeRaw) / clockFreq * 1000000000.;
		    t2fine[0] = someVar+(timeBVal-32)*0.75; // last falling edge in ns
		    if (theConfiguration->muonSpeed)
		      {
			if ((theConfiguration->muonSpeed&30) == 2)
			  speedT2f = someVar + (timeBVal-32)*0.75;
			if ((theConfiguration->muonSpeed&480) == 2)
			  speedT1f = someVar + (timeBVal-32)*0.75;
		      }
		  }

                  for (k = 1; k < 4; k++)
		    {
		      i++;                      // look at RE and FE
		      timeA[0] = dataline[i++]; // times to determine
		      timeA[1] = dataline[i++]; // if an event took place
		      timeA[2] = (char)NULL;
		      i++;
		      timeB[0] = dataline[i++];
		      timeB[1] = dataline[i++];
		      timeB[2] = (char)NULL;
		      sscanf(timeA, "%xl", &timeAVal);
		      sscanf(timeB, "%xl", &timeBVal);

		      if (timeAVal & 32) // rising edge found!
			{
			  aHitTime = ((double)eventTimeRaw) / 41.666369;
			  if (t1fine[k] == 0.)  { // have not recorded a hit yet
			    t1fine[k] = aHitTime*1000. + (timeAVal-32)*0.75;  // precise time in ns
			  }
			  // check to see if ANY counter has been hit in the event
			  // AND if this hit is far enough after first hit to be a muon decay
			  if ( (evtSingleNew != 0)
			       && ( (aHitTime-firstHitTime) > gateAwareDecayThresh ) )  // muon decay
			    {
			      if ( muonDeltaT[k] == 0 ) // only do this for the first "muon decay" hit in a given paddle
				{
				  t2[k] = aHitTime;
				  muonDeltaT[k] = aHitTime-firstHitTime;
				  if (theConfiguration->debug)
				    fprintf(outfile, "DEBUG: Tagged as muon decay, paddle %i\n, delta-T %f,%f,%f" ,
					    k+1,muonDeltaT[k],aHitTime,firstHitTime );
				  if (evtMultipleNew == 0) // if this is the first "muondecay" hit of the event
				    {
				      evtMultipleNew |= (1 << k) ; // set multiple hit bits
				      uHistIndex = (int)(muonDeltaT[k] * 5);
				      if (uHistIndex > 99) uHistIndex = 99;
				      if (uHistIndex < 0) uHistIndex = 0;
				      muonHist[uHistIndex]++;
				      muonDecays[k]++;
				      if (hipad == k+1 || hipad == k)
					{
					  decayHist[uHistIndex]++;
					  if (hipad == theConfiguration->muon - 1)
					    {
					      absHist[uHistIndex]++;
					      if (theConfiguration->muon == k+1)
						upHist[uHistIndex]++;
					      else if (theConfiguration->muon == k+2)
						downHist[uHistIndex]++;
					    }
					} // hipad?
				    } // is this the first multiple hit anywhere?  histogram
				} // first muondecay for this paddle?
			    } // muondecay candidate
			  else  // it is a new event
			    {
			      t1[k] = ((double)eventTimeRaw) / 41.666369;
			      if (evtSingleNew == 0) firstHitTime = t1[k]; // set the global first time of the event
			      evtSingleNew |= 1 << k; // set bit for channel 1
			      if (theConfiguration->muonSpeed == 1)
				{
				  if (speedT1 == 0.0)
				    {
				      if (speedT2 == 0.0)
					speedT2 = t1[k]*1000 + (timeAVal-32)*0.75;
				      else
					speedT1 = t1[k]*1000 + (timeAVal-32)*0.75;
				    }
				}
			      else if (theConfiguration->muonSpeed > 1)
				{
				  if ((theConfiguration->muonSpeed&30)>>1 == 1<<k)
				    speedT2 = t1[k]*1000 + (timeAVal-32)*0.75;
				  if ((theConfiguration->muonSpeed&480)>>5 == 1<<k)
				    speedT1 = t1[k]*1000 + (timeAVal-32)*0.75;
				}
			      if (hipad < k+1) hipad = k+1;
			    } // new event
			} // rising edge
		      if (timeBVal & 32) { // falling edge
			someVar = ((double)eventTimeRaw) / clockFreq * 1000000000.;
			t2fine[k] = someVar+(timeBVal-32)*0.75; // last falling edge in ns
			{
			  if (theConfiguration->muonSpeed)
			    {
			      if ((theConfiguration->muonSpeed&30)>>1 == 1<<k)
				speedT2f = someVar + (timeBVal-32)*0.75;
			      if ((theConfiguration->muonSpeed&480)>>5 == 1<<k)
				speedT1f = someVar + (timeBVal-32)*0.75;
			    }
			}
		      }
		    } // loop over all paddles (except #1... numbered 0 here... oh my head hurts...)
                  if (insideEvent == 1 && first2 == 0)
                  {


// Jordan's GPS code begins here
/*
If I remember correctly, dNano is the key variable, and the one that caused me the most trouble.

Here's the basic idea behind this section of code. First, it reads all of the GPS related info straight
from the raw dataline, and it stores each important hexidecimal number as some variable. Next it rounds
the numbers appriately, so you don't end up with readouts of times like 60.123234 seconds. Instead, it
should round up to the next minute. The most important part of the code, however, is the section towards
the end that is designed to calculate dNano--or the decimal portion of the absolute time that is displayed.
For example, a time readout may look like 11:42:54.23522434. In this case, dNano should be .23522434. I tried
to make my calculation of dNano essentially identical to the examples that I saw in the manual that you gave
me. The problem is that in order for my algorithm to make sense, the pps time must always be larger then the
trigger time. If not, then dNano calculates to a number greater then 1. Then, when dNano is added to second
in the last line of my code, (second is meant to only represent the absolute second to the nearest whole number),
the absolute time readout is thrown off. I tried to make my documentation a little bit more detailed... so
hopefully that will help.

It's also worth pointing out that the "dataInvalid" problem was only noticeable last summer when we ran the
detectors at a relatively high frequency. As long as the data came in at a reasoneable rate (indoors), we would
never (not even rarely) see invalid data.
*/
if (theConfiguration->useGPS) {

	char tempStr[10], sign;
		// tempStr stores little bits of the dataline as they are being assigned to different variables
	unsigned long trigger, pps, r1, r2, r3, r4; //these all store parts of the dataline
	int i, j, second, delay, fineAdjust, GPSdebug1,
	    GPSdebug2, GPSdebug3, compHour, n;
	double dNano, dNum, dDenom, k, fineAdjuster;
		//dNum and dDenom are used in calculating dNano, since essentially it is just a fraction

	GPSvalid=newGPSvalid;
	DATAvalid=newDATAvalid;
	newGPSvalid = 1; // switches to 0 if the dataline states GPS is invalid
	newDATAvalid = 1; 	// switches to 0 if GPS info in the dataline does not make
					// sense to me according to my algorithm.... i.e. if the
					// variable dNano
	for (n = 0; dataline[n] == ' ' || dataline[n] == '>'; n++);

	// check for validity
	if (dataline[60+n] == 'V') newGPSvalid = 0;

	GPSdebug1 = 0; GPSdebug2 = 0; GPSdebug3 = 0; 
	// GPSdebug1 = 1; GPSdebug2 = 1; GPSdebug3 = 1; 

	fineAdjust = 0;

	if (GPSdebug1 || GPSdebug2 || GPSdebug3) printf(dataline);



	// SET VARIABLES TO INFO IN dataline
	for (i = 0, j = 0; j < 8; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%8x", & trigger);

	for (i = 9, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%2x", & r1);
	for (i = 15, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%2x", & r2);
	for (i = 21, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%2x", & r3);
	for (i = 27, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%2x", & r4);

	for (i = 33, j = 0; j < 8; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%8x", & pps);
	for (i = 42, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	GPShour = newGPShour;
	sscanf(tempStr, "%2d", & newGPShour);
	for (i = 44, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	GPSminute = newGPSminute;
	sscanf(tempStr, "%2d", & newGPSminute);
	for (i = 46, j = 0; j < 5; i++) {
		if (dataline[i+n] == '.');
		else {tempStr[j] = dataline[i+n]; j++;}
	}
	sscanf(tempStr, "%5d", & second);
	for (i = 62, j = 0; j < 2; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%2d", & numSats);
	sign = dataline[67+n];
	for (i = 68, j = 0; j < 4; i++, j++) tempStr[j] = dataline[i+n];
	sscanf(tempStr, "%4d", & delay);
		// END SETTING VARIABLES TO dataline



	if (GPSdebug1) {
		fprintf(outfile,"%x %2x    %2x    %2x    %2x    ", trigger, r1, r2, r3, r4);
		fprintf(outfile,"%x %d%d%d                ", pps, GPShour, GPSminute, second);
		fprintf(outfile,"%c%4d\n", sign, delay);
	}


	// I don't exactly remember the purpose of "lapped". (I believe it had something to do with
	// calculating an average value for dDenom later on). However, I know that this if statement
	// is here just to adjust the hour into the desired time zone.
	if (lapped < 1) {
	  theTime = time(0);
	  strftime(tempStr, 4, "%H", localtime(&theTime));
	  if (tempStr[0] == '0') {
	    tempStr[0] = tempStr[1];
	    sscanf(tempStr, "%1d", &compHour);
	  }
	  else sscanf(tempStr, "%2d", &compHour);
	  hourOffset = GPShour - compHour;
	  lapped = 1;
	}
	if (theConfiguration->timeFix) GPShour -= hourOffset; // fixes hour to puter's time zone



	// BEGIN ROUNDING OF SECOND

	// calculates variable second based on raw data
	if (sign == '+') second += delay;
	else if (sign == '-') second -= delay;
	else printf("ERR: delay sign not valid \n");
	if (GPSdebug2) fprintf(outfile,"second + delay = %d", second);

	// rounds off second to nearest whole number
	i = second % 1000;
	if (i < 500) second -= i;
	else {second -= i; second += 1000;}
	second /= 1000;


	if (GPSdebug2) {
		fprintf(outfile," ---> %d, decimal = %d\n", second, i);
	}


	// checks for validity of dataline
	// original comparison (JW 2006) was
	//   if (trigger < pps) DATAvalid = 0;
	// but this fails frequently at high pulse rates and always
	// at pulse rates > 25 Hz.  
	// for the moment, just remove this check

	if ( !(trigger >= pps) && !(trigger < pps) ) DATAvalid = 0;



	// ADJUST TRIGGER AND GET dNano

	dNum =  ((double) trigger) - ((double) pps); // finds the numerator of the fraction dNano

	// if more than twenty seconds, likely
	// this is a case of rollover of one of the counters
	// so correct for that
	if (abs(dNum) > clockFreq*20.) {
	  if (dNum > 0) dNum = dNum - + pow(2,32);
	  else dNum = dNum + pow(2,32) ;
	}


	// I don't remember exactly why fine adjustment was necessary... but not likely the root of the problem
	if (fineAdjust) {
	  if (evtSingle &1) fineAdjuster = ((double) r1) - 160.;
	  else if (evtSingle & 2) fineAdjuster = ((double) r2) - 160.;
	  else if (evtSingle & 3) fineAdjuster = ((double) r3) - 160.;
	  else if (evtSingle & 4) fineAdjuster = ((double) r4) - 160.;
	  fineAdjuster /= 32.0;
	  dNum += fineAdjuster;
	}


	// if (dNum < 0) printf("ERR: invalid trigger or pps\n");

	// I left two options for calculating dDenom. One used some sort of average, while the other
	// had just used data from a single event. Either way, I believe dDenom was meant to remane
	// somewhat constant.
	if (!theConfiguration->useAvgFreq) { // uses previous data to find an accurate denom
	  if (lapped > 1) {
	    dDenom = ((double) pps) - ((double) lastPPS);
	    for (k = 1.0; ((dDenom/k)/clockFreq) > 1.3; k += 1.0);
	    dDenom /= k;
	    if (dDenom < 10000.0) dDenom = lastDenom;
	  }
	  else dDenom = avgDenom;
	  //resets variables
	  lastPPS = pps;
	  lastDenom = dDenom;
	  lapped = 2;
	}
	else dDenom = avgDenom;

	//finds dNano fraction
	dNano = dNum / dDenom;
		// END FINDING dNano



	if (GPSdebug3) {
		fprintf(outfile,"trigger = %u or %x, ", trigger, trigger);
		fprintf(outfile,"pps = %u or %x, ", pps, pps);
		fprintf(outfile,"fadj = %u\n", fineAdjuster);
		fprintf(outfile,"num = %8.1lf, denom = %8.1lf, nano = %2.10lf\n", dNum, dDenom, dNano);
	}


	// display or set the goods
	if (lapped) GPSsecond = newGPSsecond;
	newGPSsecond = dNano + second;

	
	//// old code for rounding off time to avoid 
	//// second values that don't fall between 0 and 60
	//if (second < 0) {second += 60; GPSminute--;}
	//else if (second >= 60) {second -= 60; GPSminute++;}
	//if (GPSminute < 0) {GPSminute += 60; GPShour--;}
	//else if (GPSminute >= 60) {GPSminute -= 60; GPShour++;}
	//if (GPShour < 1) {GPShour += 24;}
	//else if (GPShour > 24) {GPShour -= 24;}
	///// END ROUNDING OF SECOND


	if (GPSdebug3) fprintf(outfile,"time before rounding correction:%2i:%2i:%2.10f\n",GPShour,GPSminute,GPSsecond);
 
	if (GPSsecond >= 60.) {
	  GPSsecond -= 60. ;
	  if ((GPSminute++) > 59) {
	    GPSminute -= 60;
	    if ((GPShour++) > 23) GPShour += 24;
	  }
	}
	if (GPSsecond < 0.) {
	  GPSsecond += 60. ;
	  if ((GPSminute --) <0) {
	    GPSminute += 60;
	    if ((GPShour--) <0) GPShour += 24;
	  }
	}
	if (GPSdebug3) fprintf(outfile,"time after rounding correction:%2i:%2i:%2.10f\n",GPShour,GPSminute,GPSsecond);

}
// Jordan's GPS code ends here
//



                    insideEvent = 0;
		    gettimeofday(&preciseTime,&tzone);
		    prevWallEventTime=wallEventTime;
		    wallEventTime= preciseTime.tv_sec;
		    wallEventTime += preciseTime.tv_usec/1000000.;
                    prevTime = eventTime;
                    eventTime = eventTimeRaw/clockFreq;
		    if (theConfiguration->debug) printf("DEBUG ROLLOVER:\n Wall Times: %f\t%f\n CPLD times: %f\t%f\n gettimeofday values:%d\t%d\n",prevWallEventTime,wallEventTime,prevTime,eventTime,preciseTime.tv_sec,preciseTime.tv_usec);
		    rollTime = 0;
                    if (eventTime < prevTime) rollTime += clockRollOver; // fix for rollover
                    if (first) first = 0;
		    else {
		      // check for multiple rollover
		      while (wallEventTime - prevWallEventTime > eventTime - prevTime + rollTime + clockRollOver / 2 )
			rollTime += clockRollOver;
		      // update event time
		      runTime += eventTime - prevTime + rollTime;
		    }
		    if (theConfiguration->debug) printf(" Rollover Time:%f\n",rollTime);

                    if (theConfiguration->beep) printf("\x007");
                    eventNum++;
                    intervalEventNum++;


		    if (theConfiguration->rawGPSdata) {
		      printf("%i\t%2.9lf", eventNum, GPSsecond);
		      if (!newGPSvalid) printf("\tGPSinvalid");
		      else if (!newDATAvalid) printf("\tDATAinvalid");
		      if (theConfiguration->reportSats)
            		  printf("\t%i", numSats);
		      printf("\n");
		      if (theConfiguration->outputfile) {
			fprintf(outfile, "%i\t%2.9lf", eventNum, GPSsecond);
			if (!newGPSvalid) fprintf(outfile, "\tGPSinvalid");
			else if (!newDATAvalid) fprintf(outfile, "\tDATAinvalid");
            		if (theConfiguration->reportSats)
			  fprintf(outfile, "\t%i", numSats);
            		fprintf(outfile, "\n");
		      }
		      break;
		    }

                    // format the hit output information
		    // note that evtSingle (information about the PREVIOUS event
		    //  can't be printed and finalized until the new events is
		    //  seen... otherwise, how do we know it is new?
		    // Oh the tangled webs...

                    sprintf(output, "(");
                    if (evtSingle & 1) strcat(output, "1,");
                    else strcat(output, "-,");
                    if (evtSingle & 2) strcat(output, "2,");
                    else strcat(output, "-,");
                    if (evtSingle & 4) strcat(output, "3,");
                    else strcat(output, "-,");
                    if (evtSingle & 8) strcat(output, "4)");
                    else strcat(output, "-)");

		    // rise and fall time outputs.  Note that since
		    //  we are limiting ourselves to 5 digits, maximum
		    //  time is 99999. ns

		    for (k=0; k<4; k++) {
		      disptime[k]=oldt1fine[k];
		      if (disptime[k]<0.) disptime[k]=0.;
		      if (disptime[k]>99999.) disptime[k]=99999.;
		    }
                    sprintf(edgeOutput, "<");
		    sprintf(edgeTemp,"%7.1f,",disptime[0]);
                    if (evtSingle & 1) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f,",disptime[1]);
                    if (evtSingle & 2) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f,",disptime[2]);
                    if (evtSingle & 4) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f> <",disptime[3]);
                    if (evtSingle & 8) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-> <");
		    for (k=0; k<4; k++) {
		      disptime[k]=oldt2fine[k];
		      if (disptime[k]<0.) disptime[k]=0.;
		      if (disptime[k]>99999.) disptime[k]=99999.;
		    }

		    sprintf(edgeTemp,"%7.1f,",disptime[0]);
                    if (evtSingle & 1) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f,",disptime[1]);
                    if (evtSingle & 2) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f,",disptime[2]);
                    if (evtSingle & 4) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "-,");
		    sprintf(edgeTemp,"%7.1f>",disptime[3]);
                    if (evtSingle & 8) strcat(edgeOutput, edgeTemp);
                    else strcat(edgeOutput, "->");

		    //so that the edges are easily used in excel!
		    for (k=0; k<4; k++) {
		      disptime[k]=oldt1fine[k];
		      if (disptime[k]<0.) disptime[k]=0.;
		      if (disptime[k]>99999.) disptime[k]=99999.;
		    }
		    sprintf(edgeOutputFile, "RisingEdge");
		    sprintf(edgeTemp, "\t %7.1f", disptime[0]);
		    if (evtSingle & 1) strcat(edgeOutputFile, edgeTemp);
		    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f",disptime[1]);
                    if (evtSingle & 2) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f",disptime[2]);
                    if (evtSingle & 4) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f \t FallingEdge",disptime[3]);
                    if (evtSingle & 8) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t- \t FallingEdge");

		    for (k=0; k<4; k++) {
		      disptime[k]=oldt2fine[k];
		      if (disptime[k]<0.) disptime[k]=0.;
		      if (disptime[k]>99999.) disptime[k]=99999.;
		    }

		    sprintf(edgeTemp,"\t %7.1f",disptime[0]);
                    if (evtSingle & 1) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f",disptime[1]);
                    if (evtSingle & 2) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f",disptime[2]);
                    if (evtSingle & 4) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    sprintf(edgeTemp,"\t %7.1f",disptime[3]);
                    if (evtSingle & 8) strcat(edgeOutputFile, edgeTemp);
                    else strcat(edgeOutputFile, "\t-");
		    //okay so I am done messing around

                    evtList[evtSingle]++;  // increment event counter
                    intervalEvtList[evtSingle]++;

                    printedSomething = 0;
                    if (theConfiguration->writeEachHit || theConfiguration->rateSummary > 0)
                    {
                      printedSomething = 1;
		      printf("Event %i \t", eventNum);
		      if (!theConfiguration->useGPS)
			printf("Time %f \t", runTime);
		      if (theConfiguration->useGPS) {
			if (GPSvalid && DATAvalid)
			  printf("GPS %d:%d:%2.9lf\t", GPShour, GPSminute, GPSsecond);
			else if (!GPSvalid) printf("GPS invalid \t");
			else printf("DATA invalid \t");
			if (theConfiguration->reportSats) printf("SATS %i \t", numSats);
		      }
                      if (theConfiguration->hexCode)
			printf("%x", evtSingle);
                      else
                        printf("%s", output);
		      if (theConfiguration->riseFallTimes)
			printf("\t%s",edgeOutput);
                    }

                    if (theConfiguration->muon)
                    {
		      // the array muonDeltaT[0] is holding the delta-T of any subsequent hits
		      // so this is the indication of a muon decay event

                      gotAMuon = 0;
                      for (j = 0; j < 4; j++) // find muon time, if any
                        if (muonDeltaT[j] > 0) gotAMuon = muonDeltaT[j];
                      // NB: we assume all the times are the same (for
                      // multiple muons)

                      if (gotAMuon != 0)
                      {
                        sprintf(uoutput, "["); // format the output string
                        if (muonDeltaT[0]) strcat(uoutput, "1,");
                        else strcat(uoutput, "-,");
                        if (muonDeltaT[1]) strcat(uoutput, "2,");
                        else strcat(uoutput, "-,");
                        if (muonDeltaT[2]) strcat(uoutput, "3,");
                        else strcat(uoutput, "-,");
                        if (muonDeltaT[3]) strcat(uoutput, "4]");
                        else strcat(uoutput, "-]");
                        printedSomething = 1;
                        if (!theConfiguration->writeEachHit)
                          printf("%s", output); // ensure hit info appears
                        printf("\t%s\tdT=%f us", uoutput, gotAMuon);
                        if (!theConfiguration->outputfile)
                          for (j = 0; j < 4; j++) muonDeltaT[j] = 0;
                      }
                    }

                    if (theConfiguration->muonSpeed)
                    {
                      if (speedT1old != 0 && speedT2old != 0)
                      {
                        printf("\tTOF=%.2f ns", speedT1old - speedT2old);
                        printedSomething = 1;
                      }
                    }

                    if (printedSomething) printf("\n");

		    if (theConfiguration->rawoutputfile)
		      fprintf(rawoutfile, "Printed Event %i\n", eventNum);

                    if (theConfiguration->outputfile)
                    {
                      printedSomething = 0;
                      if (1 || theConfiguration->writeEachHit)
                      {
                        printedSomething = 1;
			fprintf(outfile, "%i\t", eventNum);
			if (!theConfiguration->useGPS)
			  fprintf(outfile, "%f\t", runTime);
			if (theConfiguration->useGPS) {
			  if (GPSvalid && DATAvalid)
			    fprintf(outfile, "%d:%d:%2.9lf\t", GPShour, GPSminute, GPSsecond);
			  else if (!GPSvalid) fprintf(outfile, "GPSinvalid\t");
			  else fprintf(outfile, "DATAinvalid\t");
			  if (theConfiguration->reportSats) fprintf(outfile, "%i\t", numSats);
			}
                        if (theConfiguration->hexCode)
                          fprintf(outfile, "%x", evtSingle);
                        else
                          fprintf(outfile, "%s", output);
			if (theConfiguration->riseFallTimes)
			  fprintf(outfile, "\t%s",edgeOutputFile);

                      }

                      if (theConfiguration->muon)
                      {
                        if (gotAMuon != 0)
                        {
                          printedSomething = 1;
                          if (!theConfiguration->writeEachHit)
                            fprintf(outfile, "%s", output);
                          fprintf(outfile, "\t%s\t%f", uoutput, gotAMuon);
                          for (j = 0; j < 4; j++) muonDeltaT[j] = 0;
                        }
                      }

                      if (theConfiguration->muonSpeed)
                      {
                        if (speedT1old != 0 && speedT2old != 0)
                        {
                          fprintf(outfile, "\tTOF\t%.2f", speedT1old - speedT2old);
                          if (theConfiguration->debug)
                            fprintf(outfile, "\t%f\t%f\t%f\t%f", speedT2old, speedT2fold, speedT1old, speedT1fold);
                          printedSomething = 1;
                        }
			//     fprintf(outfile, "\tTOF~\t%f\t%f", speedT1old, speedT2old);
                      }

                      if (printedSomething) fprintf(outfile, "\n");

                      if (theConfiguration->rateSummary > 0)
			{
			  updateInterval=(int)runTime
			    - interval*theConfiguration->rateSummary;  // how long since last interval declared
			  while (updateInterval >= theConfiguration->rateSummary)
			    {
			      fprintf(outfile, "\t%i", interval++);  // increment and print the interval
			      intervalTime = time(0);
			      // correct this time by however long we ran over the interval!
			      intervalTime -= updateInterval - theConfiguration->rateSummary;
			      strftime(formattedDate, lengthFormattedDate, "%D\t%T",localtime(&intervalTime));
			      fprintf(outfile, "\t%s", formattedDate);
			      updateInterval=(int)runTime
				- interval*theConfiguration->rateSummary;  // update the updateInterval variable

			      // now we have to decide if events belong to this interval or to a subsequent one
			      // (relevant for low rates)

			      if (updateInterval < theConfiguration->rateSummary ) // belongs to this interval
				{
				  fprintf(outfile, "\t%i", intervalEventNum);
				  fprintf(outfile, "\t%.6f", (float)intervalEventNum/theConfiguration->rateSummary);
				  fprintf(outfile, "\t%.6f", ((float)intervalEventNum/theConfiguration->rateSummary)/sqrt((double)intervalEventNum));
				  if (theConfiguration->efficient) // print efficiency summary
				    for (j = 1; j < 16; j++)
				      fprintf(outfile, "\t%10i", intervalEvtList[j]);
				  for (j = 0; j < 16; j++)
				    {
				      intervalSumList[j] += intervalEvtList[j];  // store efficiency summary
				      intervalEvtList[j] = 0; // and clear it
				    }
				  intervalEventSum += intervalEventNum;
				  intervalEventNum = 0;  // reset counter
				} // events belonged to this interval
			      else  // do a parallel printout with zeros
				{
				  fprintf(outfile, "\t%i", 0);
				  fprintf(outfile, "\t%.6f", 0.);
				  fprintf(outfile, "\t%.6f", 0.);
				  if (theConfiguration->efficient) // print efficiency summary
				    for (j = 1; j < 16; j++)
				      fprintf(outfile, "\t%10i", 0);
				}

			      if (theConfiguration->rateSummaryGoodies)
				{
				  long int begintime, seconds;
				  if (gettimeofday(&preciseTime,&tzone)); // error handling TBC
				  begintime = preciseTime.tv_usec;
				  seconds = preciseTime.tv_sec;
				  // note... can't correct this time for when interval ended!
				  fprintf(outfile, "\t%.2f", readBarometer(serial, main_queue_id, theConfiguration->barometerCalibrated, theConfiguration->rawBoard, outfile));
				  fprintf(outfile, "\t%.2f", readThermometer(serial, main_queue_id));
				  if (gettimeofday(&preciseTime,&tzone)); // error handling TBC
				  if (preciseTime.tv_sec == seconds)
				    timeFix += (float)(preciseTime.tv_usec - begintime)/1000000;
				  else
				    timeFix += 1+(float)(preciseTime.tv_usec - begintime)/1000000;
				  if (theConfiguration->debug)
				    printf("DEBUG: time fix: %f\n", timeFix);
				} // "goodies"
			      fprintf(outfile, "\n");  // end the
			    }
                        } // rate summaries
		      fflush(outfile);
                    }
                  }
                  break;
                default: break;
              } // end switch
            } // end while

	    serialSend(serial, "CD\r"); // stop the fifo output

            runTime -= timeFix;  // account for BA TH delay, if any

            // stop the timer
            msg->mtype = END;
            sprintf(msg->mtext, timestr);
	    sprintf(filename, "timestamp");
            if (theConfiguration->debug)
              printf("Main:About to stop timer\n");
            rc = msgsnd(time_queue_id, msg, strlen(msg->mtext)+1, 0);
            if (theConfiguration->debug)
              printf("Main:Timer message sent.\n");
            if (rc == -1)
            {
              perror("main: msgsnd");
              if (theConfiguration->debug) printf("Errno is %i\n", errno);
            }

if (!theConfiguration->rawGPSdata) {

            printf("\nRun ended on %s\n", ctime(&theTime));
            printf("\nSum over entire run:\n");
            if (theConfiguration->debug)
              printf("(-,-,-,-): %i\n", evtList[0]);
            printf("(1,-,-,-): %i\n", evtList[1]);
            printf("(-,2,-,-): %i\n", evtList[2]);
            printf("(-,-,3,-): %i\n", evtList[4]);
            printf("(-,-,-,4): %i\n", evtList[8]);
            printf("(1,2,-,-): %i\n", evtList[3]);
            printf("(1,-,3,-): %i\n", evtList[5]);
            printf("(1,-,-,4): %i\n", evtList[9]);
            printf("(-,2,3,-): %i\n", evtList[6]);
            printf("(-,2,-,4): %i\n", evtList[10]);
            printf("(-,-,3,4): %i\n", evtList[12]);
            printf("(1,2,3,-): %i\n", evtList[7]);
            printf("(1,-,3,4): %i\n", evtList[13]);
            printf("(-,2,3,4): %i\n", evtList[14]);
            printf("(1,2,-,4): %i\n", evtList[11]);
            printf("(1,2,3,4): %i\n", evtList[15]);
            printf("%i events", eventNum);
	    if (theConfiguration->useGPS)
	      printf("\n\n");
	    else if (theConfiguration->rateSummaryGoodies || done == 2)
              printf(" in %i seconds.\n\n", (int)runTime);
            else
              printf(" in %s seconds.\n\n", timestr);

            if (theConfiguration->muon)
            {
              printf("Muon decay (double hit) counts:\n");
              printf("Channel 1: %i\n", muonDecays[0]);
              printf("Channel 2: %i\n", muonDecays[1]);
              printf("Channel 3: %i\n", muonDecays[2]);
              printf("Channel 4: %i\n", muonDecays[3]);
            }
}
            if (theConfiguration->outputfile)
            {
              fprintf(outfile, "\nRun ended on %s\n", ctime(&theTime));
              if (theConfiguration->efficient && !theConfiguration->rawGPSdata)
              {
                if (theConfiguration->rateSummary)
                {
                  fprintf(outfile, "\nSum over all intervals:\n");
                  if (theConfiguration->debug)
                    fprintf(outfile, "(-,-,-,-): %i\n", intervalSumList[0]);
                  fprintf(outfile, "(1,-,-,-): %i\n", intervalSumList[1]);
                  fprintf(outfile, "(-,2,-,-): %i\n", intervalSumList[2]);
                  fprintf(outfile, "(-,-,3,-): %i\n", intervalSumList[4]);
                  fprintf(outfile, "(-,-,-,4): %i\n", intervalSumList[8]);
                  fprintf(outfile, "(1,2,-,-): %i\n", intervalSumList[3]);
                  fprintf(outfile, "(1,-,3,-): %i\n", intervalSumList[5]);
                  fprintf(outfile, "(1,-,-,4): %i\n", intervalSumList[9]);
                  fprintf(outfile, "(-,2,3,-): %i\n", intervalSumList[6]);
                  fprintf(outfile, "(-,2,-,4): %i\n", intervalSumList[10]);
                  fprintf(outfile, "(-,-,3,4): %i\n", intervalSumList[12]);
                  fprintf(outfile, "(1,2,3,-): %i\n", intervalSumList[7]);
                  fprintf(outfile, "(1,-,3,4): %i\n", intervalSumList[13]);
                  fprintf(outfile, "(-,2,3,4): %i\n", intervalSumList[14]);
                  fprintf(outfile, "(1,2,-,4): %i\n", intervalSumList[11]);
                  fprintf(outfile, "(1,2,3,4): %i\n", intervalSumList[15]);
                  fprintf(outfile, "%i events.\n", intervalEventSum);
                  fprintf(outfile, "Average interval time: %f seconds.\n\n", runTime / interval);
                }
                fprintf(outfile, "\nSum over entire run:\n");
                if (theConfiguration->debug)
                  fprintf(outfile, "(-,-,-,-): %i\n", evtList[0]);
                fprintf(outfile, "(1,-,-,-): %i\n", evtList[1]);
                fprintf(outfile, "(-,2,-,-): %i\n", evtList[2]);
                fprintf(outfile, "(-,-,3,-): %i\n", evtList[4]);
                fprintf(outfile, "(-,-,-,4): %i\n", evtList[8]);
                fprintf(outfile, "(1,2,-,-): %i\n", evtList[3]);
                fprintf(outfile, "(1,-,3,-): %i\n", evtList[5]);
                fprintf(outfile, "(1,-,-,4): %i\n", evtList[9]);
                fprintf(outfile, "(-,2,3,-): %i\n", evtList[6]);
                fprintf(outfile, "(-,2,-,4): %i\n", evtList[10]);
                fprintf(outfile, "(-,-,3,4): %i\n", evtList[12]);
                fprintf(outfile, "(1,2,3,-): %i\n", evtList[7]);
                fprintf(outfile, "(1,-,3,4): %i\n", evtList[13]);
                fprintf(outfile, "(-,2,3,4): %i\n", evtList[14]);
                fprintf(outfile, "(1,2,-,4): %i\n", evtList[11]);
                fprintf(outfile, "(1,2,3,4): %i\n", evtList[15]);
                fprintf(outfile, "%i events", eventNum);
		if (theConfiguration->useGPS) fprintf(outfile, "\n\n");
		else if (theConfiguration->rateSummaryGoodies || done == 2)
                  fprintf(outfile, " in %i seconds.\n\n", (int)runTime);
                else
                  fprintf(outfile, " in %s seconds.\n\n", timestr);

                if (theConfiguration->muon)
                {
                  fprintf(outfile, "\nMuon decay (double hit) counts:\n");
                  fprintf(outfile, "Channel 1: %i\n", muonDecays[0]);
                  fprintf(outfile, "Channel 2: %i\n", muonDecays[1]);
                  fprintf(outfile, "Channel 3: %i\n", muonDecays[2]);
                  fprintf(outfile, "Channel 4: %i\n", muonDecays[3]);
                  fprintf(outfile, "\nMuon decay histogram (.2 us/bin):\n");
                  for (j = 0; j < 100; j++)
                    fprintf(outfile, "%i\t : %i\n", j, muonHist[j]);
                  fprintf(decayfile, "\nMuon sequential decay histogram (.2 us/bin):\n");
                  for (j = 0; j < 100; j++)
                    fprintf(decayfile, "%i\t : %i\n", j, decayHist[j]);
                  if (theConfiguration->muon > 1)
                  {
                    fprintf(absfile, "\nMuon absorber decay histogram (.2 us/bin):\n");
                    for (j = 0; j < 100; j++)
                      fprintf(absfile, "%i\t : %i\n", j, absHist[j]);
                    fprintf(upfile, "\nMuon absorber upward decay histogram (.2 us/bin):\n");
                    for (j = 0; j < 100; j++)
                      fprintf(upfile, "%i\t : %i\n", j, upHist[j]);
                    fprintf(downfile, "\nMuon absorber downward decay histogram (.2 us/bin):\n");
                    for (j = 0; j < 100; j++)
                      fprintf(downfile, "%i\t : %i\n", j, downHist[j]);
                  }

                  // restore default .24 us gate
                  serialSend(serial, "WC 2 2A\r");
                  usleep(10000);
                  serialSend(serial, "WC 3 0\r");
                  usleep(10000);
                }
              }
              fclose(outfile);
              if (theConfiguration->muon) fclose(decayfile);
              if (theConfiguration->muon > 1)
              {
                fclose(absfile);
                fclose(upfile);
                fclose(downfile);
              }
            }
	    if (theConfiguration->rawoutputfile) {
	      fprintf(rawoutfile, "\n%i EVENTS\nRun Ended: %s\n", eventNum, ctime(&theTime));
	      fclose(rawoutfile);
	    }


            if (choicechar == '4')
            {
              sprintf(output, "WC 0 %x\r", oldCR);
              serialSend(serial, output);
              usleep(10000);
            }
            break;
          }
          else // version 0 board
          {
            time_t starttime,stoptime;
            time_t startstamp,curtime,laststamp,rateTime;
            FILE *outfile ;
            int spos,first,j, totalhits;
            time_t endTime;

            //open files for muon run
            FILE *noise ;
            FILE *upward ;
            FILE *downward ;

            /* Added for Muon Decay 6-02 ASH ----------*/
            //proper muon decay must have at least 19 characters
            //need varibles for 2nd hit discription and 2nd hit time

            char secondhitstr[100];
            char secondtimestr[100];

            //varibles to hold the hex code
            unsigned long muhit, mutime;

            //clear arrays

            while (i<=100) {
              secondhitstr[i] = 0;
              secondtimestr[i] = 0;
              i = ++i;
            }

            /*----------------------------------------*/

            eventcounter = 0;
            nInterval = 0; // counts in an interval
            totaltime = 0;
            rateInterval=0;
            previousRateInterval=rateInterval;


            printf ("How long do you want the run to proceed?\n");
            printf ("Enter answer in seconds.\n");

            receiveKeyString(timestr);


            //  	  /* receive a message of KEY_TYPE */
            //  	  rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, KEY_TYPE, 0);
            //  	  if (rc == -1) {
            //  	    perror("main: msgrcv");
            //  	    exit(1);
            //  	      }
            //  	  sprintf(timestr, msg->mtext);



            starttime=time(0);
            strftime(timestampstr,timesize,"%m-%d-%y--%H-%M-%S",
                     localtime(&starttime));

            if (strcmp(filename,"timestamp")==0)
              sprintf(suffix,timestampstr);
            else
              sprintf(suffix,filename);

            sprintf(filestr,"%s/muon.%s",getenv("HOME"),suffix);
            /* prepare file names for muon decay run */
            if (theConfiguration->muon)
              {
                sprintf(downstr,"%s/downward.%s",
                        getenv("HOME"),suffix);
                sprintf(upstr,"%s/upward.%s",
                        getenv("HOME"),suffix);
                sprintf(noisestr,"%s/noise.%s",
                        getenv("HOME"),suffix);
              }

            if (theConfiguration->outputfile){
              printf("Opening output file %s\n",filestr);
              outfile = fopen(filestr,"w");
	      if ( outfile == NULL )
		{
		  //printf(" Failed to open file %s, error:%s\n",filestr,sys_errlist[errno]);
		  printf(" Failed to open file %s, error:%s\n",filestr,strerror(errno));
		  break;
		}

              /* Open Files for muon decay run */
              if (theConfiguration->outputfile &&
                  theConfiguration->muon)
                {
                  printf("Opening output file %s\n",downstr);
                  downward = fopen(downstr,"w");
		  if ( downward == NULL )
		    {
		      //printf(" Failed to open file %s, error:%s\n",downstr,sys_errlist[errno]);
		      printf(" Failed to open file %s, error:%s\n",downstr,strerror(errno));
		      break;
		    }
                  printf("Opening output file %s\n",upstr);
                  upward = fopen(upstr, "w");
		  if ( upward == NULL )
		    {
		      //printf(" Failed to open file %s, error:%s\n",upstr,sys_errlist[errno]);
		      printf(" Failed to open file %s, error:%s\n",upstr,strerror(errno));
		      break;
		    }
                  printf("Opening output file %s\n",noisestr);
                  noise = fopen(noisestr,"w");
		  if ( noise == NULL )
		    {
		      //printf(" Failed to open file %s, error:%s\n",noisestr,sys_errlist[errno]);
		      printf(" Failed to open file %s, error:%s\n",noisestr,strerror(errno));
		      break;
		    }
                }

              fprintf(outfile,"Run Started at %s\n",ctime(&starttime));

              /*fprintf(outfile, "Totaltime (using %% l u ) begins at %lu\n",
                totaltime);*/
            }

            // Step 1a:  tell time sender to start counting


            // send the "begin run" message to the time sender
            //
            msg->mtype = BEGIN;
            sprintf(msg->mtext, timestr);
            rc = msgsnd(time_queue_id, msg, strlen(msg->mtext)+1, 0);

            if (rc == -1) {
              perror("main: msgsnd");
              done = 1;
            }

            // Step 1b:  Write to the board to begin the run.


            //serialSend(serial,"NE\r");
            serialSend(serial,"EC\r");
            serialSend(serial,"WC 0\r");
            serialSend(serial,"RS\r");

            serialSend(serial,"WI 3 1\r");
            serialSend(serial,"WW 1\r");
            serialSend(serial,"ES\r");

            /* to run the Muon Lifetime need to suppress singles */
            if (theConfiguration->muon){
              /*Appears to be working-------ASH 6-02-----------*/
              /*serialSend(serial, "RS\r");
                serialSend(serial, "SS\r");
                serialSend(serial, "WW 1\r");
                serialSend(serial, "WC 1f\r");*/
              /*Reduce to minimal, allow configuration, KSM 6 Aug 02 */
              serialSend(serial, "SS\r");
            }

            if (theConfiguration->version == 0){
              serialSend(serial,"WD 3FFB FF\r");
              interval = .000008; /* hard-wire interval, WD 3FFB FF */
            }
            else {
              serialSend(serial,"TP 6\r");
              interval = 64.e0/6.e+6; /* 64/(6MHz) clock rate */
            }

            //Muon interval
            muinterval = interval/256;
            //printf("muinterval = %1.12f\n",muinterval);
            //printf("int =%f aka = %1.12f\n",interval,interval/256);

            // Before starting, clear all messages from the main queue.
            if (theConfiguration->debug)
              printf ("Main:I am going to clear the queue now.\n");

            rc = 0;
            while ( (errno != ENOMSG) || (rc != -1) ) {
              rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, 0, IPC_NOWAIT);
              if (theConfiguration->debug){
                printf("Main:rc = %i      ", rc);
                printf("Main:Errno = %i    ", errno);
                printf("Main:ENOMSG = %i    ", ENOMSG);
                printf("Main:     Removing old Q....\n");
              }
            }

            if (choicechar == '5') /* custom run */
              {
#ifndef FAKECUSTOM
                /* this is the way this should be done; however
                   it doesn't work!  for some reason the last digit
                   is ignored!! */
                /* ksm 11/10/00 -- make two changes:
                   (1) fix bug that was attempting to capitalize
                   string termination character
                   (2) remove \r from message string and put
                   explicitly in fprintf, as with WCF case
                */
                sprintf(message,"WC %04x\r",CRegister);
                serialSend(serial,message);
#else
                /* run as a filter */
                serialSend(serial,"WC F\r");
#endif
                channelMask = CRegister & 0xF;
                coinLevel = 1+ (CRegister & 0x30)/0x10;
                veto = (CRegister & 0xc0)/0x40;
              }
            else /* default coincidence run (filter) */
              {
#ifndef FAKECUSTOM
                serialSend(serial,"WC 1F\r");
#else
                serialSend(serial,"WC F\r");
#endif
                channelMask = 0xF;
                coinLevel = 2;
                veto = 0;
              }


            // Step 2:  Read from the main queue

            first=1;
            totaltime=0;
            printf("Run active; enter '6' to quit\n");
            /* form a loop of receiving messages and printing them out. */
            done = 0;
            while (!done) {
              /* receive a message of any type */
              // printf("Blocking for any message\n");
              rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, 0, 0);
              if (theConfiguration->debug)
                printf("Message received line 375.\n");
              if (rc == -1) {
                perror("main: msgrcv");
                done = 1;
              }

              switch(msg->mtype){

              case KEY_TYPE:   // Keyboard

                {
                  sprintf(dataline, msg->mtext);
                  printf("%s",dataline);
                  if  ( dataline[0] == '6' )  {
                    printf("Aborting the run...\n");
                    if (theConfiguration->outputfile)
                      fprintf(outfile, "Run prematurely aborted.\n");
                    done = 1;
                  }
                  else {
                    printf("Invalid command, only (6) is valid during run.\n");
                  }
                  break;
                }

              case TIME_TYPE:   // timer

                {
                  // Note:  a signal from the timer always indicates
                  //          a command to stop run.
                  done = 1;
                  break;
                }

              case SERIAL_TYPE:   // serial port data
                {
                  unsigned char paddlemask,veto;
                  int paddlecount, coinLevel3, inhibit, bits;
                  int dffstamp,stamptime;
                  int theInterval, deltaEff;
		  int downFlag,upFlag,noiseFlag;

                  if (theConfiguration->debug){
                    printf("Main:line __LINE__:Reading from serial port\n");
                    // print to screen so user can see it
                    if (theConfiguration->outputfile)
                      fprintf(outfile, "msg->mtext is %s\n", msg->mtext);
                  }

                  curtime=time(0); // store time that message arrived
                  sprintf(dataline, msg->mtext);

                  // check to make sure all characters are either whitespace or hex digits
                  badchar = 0;
                  i = 0;
                  if (theConfiguration->debug && theConfiguration->outputfile)
                    fprintf(outfile, "The length of dataline is %i\n", strlen(dataline));
                  /*-------------------removed--------------------*/
                  // make it easier to read the output
                  // usleep(100000);
                  //  	      while (!( i >= strlen(dataline)) ) {
                  //  		test[0] = dataline[i];
                  //  		test[1] = (char) NULL;
                  //  		if (strstr("0123456789abcdefABCDEF ", test) == NULL) {
                  //  		  badchar = 1;
                  //  		  printf("Badchar found at character %i\n", i);
                  //  		}
                  //  	      }

                  /*----------------Continue---------------------*/

                  // proper output must have at least 11 characters
                  if (strlen(dataline) < 11) {
                    badchar = 1;
                    if (theConfiguration->debug
                        && theConfiguration->outputfile)
                      fprintf (outfile, "Line too short; string omitted.\n");
                  }

                  //test info----------------------------------------
                  /*if (strlen(dataline) >= 11){
                    printf("dataline = %s\n", dataline);
                    }*/
                  //End test info

                  if (!badchar) {
                    i = 0;
                    // clear white space at the beginning (includes prompt)
                    while (dataline[i] == ' ' || dataline[i]=='>') {
                      i++;
                    }

                    j=0;
                    while (!((dataline[i] == ' ') ||
                             ( (i+1) >= strlen(dataline)) )) {
                      test[0] = dataline[i];
                      test[1] = (char) NULL;
                      /* check to see if character is an acceptable hex */
                      if (strstr("0123456789abcdefABCDEF ", test) == NULL) {
                        badchar = 1;
                        if (theConfiguration->debug
                            && theConfiguration->outputfile)
                          fprintf(outfile, "Badchar %s found at character %i\n",test,i);
                      }
                      else {
                        timestampstr[j++] = dataline[i];
                        if (theConfiguration->debug)
                          printf("adding char %s to timestamp %i\n",test,j);
                      }
                      i++;
                    }
                    /* terminate time stamp */
                    timestampstr[j] = (char) NULL;
                    if (theConfiguration->debug)
                      printf("timestampstr after term=%s\n",timestampstr);

                    // clear white space in the middle
                    while (dataline[i] == ' ') {
                      i++;
                    }

                    j=0;
                    while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) )) {
                      test[0] = dataline[i];
                      test[1] = (char) NULL;
                      /* check to see if character is an acceptable hex */
                      if (strstr("0123456789abcdefABCDEF ", test) == NULL) {
                        badchar = 1;
                        if (theConfiguration->debug && theConfiguration->outputfile)
                          fprintf(outfile, "Badchar found at character %i\n", i);
                      }
                      else {
                        codestr[j++] = dataline[i];
                      }
                      i++;

                    }
                    /* terminate codestr */
                    codestr[j] = (char) NULL;

                    /* Muon Coincidence 6-02 ASH ------------*/

                    /* if (theConfiguration->muon){
                       printf("Muon is valid option\n");
                       }
                       else{
                       printf("No Muon Setting Detected\n");
                       }*/

                    if (theConfiguration->outputfile && theConfiguration->muon)
                      { //muon
                        //proper muon hit will have 19 characters, 18 after previous code

                        if (strlen(dataline) < 19)
                          {
                            badchar = 1;
                            if (theConfiguration->outputfile && theConfiguration->debug)
                              fprintf(outfile,"Line to short, string omitted.\n");
                          }
                        if (!badchar)
                          //remove white space after event discription
                          while (dataline[i] == ' ')
                            {
                              i++;
                            }

                        j=0;
                        while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) ))
                          {//while
                            test[0] = dataline[i];
                            test[1] = (char) NULL;
                            /* check to see if character is an acceptable hex */
                            if (strstr("0123456789abcdefABCDEF ", test) == NULL) {//if
                              badchar = 1;
                              if (theConfiguration->debug && theConfiguration->outputfile)
                                fprintf(outfile, "Badchar found at character %i\n", i);
                            }//endif
                            /*records second hit discription*/
                            else
                              {//else
                                secondhitstr[j++] = dataline[i];
                              }//endelse
                            i++;
                          }//endwhile

                        /* terminate secondhitstr */
                        secondhitstr[j] = (char) NULL;

                        //clear next amount of white space

                        while (dataline[i] == ' ') {
                          i++;
                        }

                        j=0;
                        while (!((dataline[i] == ' ') || ( (i+1) >= strlen(dataline)) )) {
                          test[0] = dataline[i];
                          test[1] = (char) NULL;
                          /* check to see if character is an acceptable hex */
                          if (strstr("0123456789abcdefABCDEF ", test) == NULL) {
                            badchar = 1;
                            if (theConfiguration->debug && theConfiguration->outputfile)
                              fprintf(outfile, "Badchar found at character %i\n", i);
                          }
                          /* record second event time */
                          else {
                            secondtimestr[j++] = dataline[i];

                          }
                          i++;
                        }
                        /* terminate secondtimestr */
                        secondtimestr[j] = (char) NULL;

                        /* Debugging Info------------------------------*/
                        //printf("dataline = %s\n", dataline);
                        //printf("second hit discription = %s\n",secondhitstr);
                        //printf("second hit time = %s\n", secondtimestr);
                        //end debug

                      }//endmuon
                    /*--End Muon Data Gathering-----------------------*/

                    if (!badchar) {

                      if (theConfiguration->debug &&
                          theConfiguration->outputfile)
                        fprintf(outfile,  "timestampstr = %s, codestr =%s, secondhitstr= %s, secondtimestr = %s\n", timestampstr, codestr,
                                secondhitstr, secondtimestr);

                      sscanf(secondhitstr, "%xl", &muhit);
                      sscanf(secondtimestr, "%xl", &mutime);
                      sscanf(timestampstr, "%xl", &timestamp);
                      sscanf(codestr, "%xl", &code);

                      if(theConfiguration->efficient){
                        int paddleMask = code & 0xF; // bitwise AND to get mask
                        efficiency[paddleMask]++;
                      }

                      /* for every piece of good data, calc time interval */
                      if (first!=0) {
                        timestamp=0;
                        startstamp=curtime;
                        laststamp=curtime;
                      }
                      difftime = timestamp*interval;

                      /*
                        want to check for a nonsense time interval
                        (empirically, this will happen in
                        very long runs)
                      */
                      stamptime = (int) (curtime-startstamp);
                      dffstamp = (int) (curtime-laststamp);
                      laststamp = curtime; // store previous time

                      /*-----------No longer Necessary, Remove?  ASH------*/
                      //detects time walk

                      if (abs(stamptime-totaltime) > 1)
                        {
                          // time walk detected... do I care?
#ifdef FIXTIMEWALK
                          printf("Time walk: int. board time %f, clock %i\n",totaltime,stamptime);
                          totaltime = stamptime; // correct to avoid more printout!
#endif
                        }
                      /* Timestamp debugging-removed bad time stamp 7-02
                         error in board code, only keeping 4 least sig. fig.
                         in time on muon run - ASH */

                      if (abs(dffstamp-difftime) > 1)
                        {
                          // bad time stamp detected
                          fakestamp =  ((float) dffstamp)/interval;
                          timestamp = fakestamp;
                          totaltime += dffstamp; /* attempt to recover */
                          // printf("Bad time stamp: board code, seconds, clock seconds %s, %f, %i\n",timestampstr,difftime,dffstamp);
                          //fprintf(outfile,"Bad time stamp: board code, seconds, clock seconds %s, %f, %i\n",timestampstr,difftime,dffstamp);
                        }

                      /*------Continue meaningful code  ASH----------*/

                      else
                        /* since all is well, add to total time */
                        totaltime += timestamp*interval;

                      /* Calculate time to electron hit
                         after inital hit(muon decay)------------ */
                      etime = totaltime+(mutime*muinterval);
                      detime = etime-totaltime;

                      /* for filtered running, set requirements */
                      paddlemask =  code & channelMask;
                      paddlecount = 0;
                      inhibit = 0;
                      for (bits=0; bits<=3; bits++)
                        if ( (paddlemask & (1<<bits)) != 0 ) {
                          paddlecount++;
                          if ((bits+1) == veto) inhibit=1;
                        }

                      //  this is the criteron for a good event!
                      if ( !inhibit && paddlecount >= coinLevel )
                        {
                          first=0; // no longer the first event
                          eventcounter++;
                          nInterval++;
                          if (theConfiguration->beep)
                            printf("\x007"); // beep to screen

                          if (theConfiguration->debug && theConfiguration->outputfile)
                            fprintf(outfile, "%s\n", dataline);
                          if (theConfiguration->debug)
                            printf("%s\n", dataline);

                          /* modified for muon liftime ASH 6-02------------------ */
                          if(theConfiguration->muon){
                            if (theConfiguration->hexCode)
			      sprintf(datasend, "%lu\t%f\t%x\t%x\t%1.12f\n",
				      eventcounter, totaltime, code, muhit, detime);
			    else
			      sprintf(datasend, "%lu\t%f\t%s\t%s\t%1.12f\n",
				      eventcounter, totaltime, decodeMask(coderep,code),
				      decodeMask(mucoderep,muhit), detime);
                          }
                          else{
                            if (theConfiguration->hexCode)
			      sprintf(datasend,"%lu\t%f\t%x\n", eventcounter, totaltime, code);
                            else
			      sprintf(datasend,"%lu\t%f\t%s\n", eventcounter, totaltime,
				      decodeMask(coderep,code));
                          }

                          /* write each event to the file if desired */

                          if (theConfiguration->outputfile &&
                              theConfiguration->writeEachHit)
                            fprintf(outfile, "%s", datasend);
                          printf("Event %s", datasend); // print to screen

                          /* Muon sorting for lifetime run */
                          /* This sort assumes a specific paddle setup, see
                             menu.c for detials*/

                          if (theConfiguration->muon &&
                              theConfiguration->outputfile)
                            //Sort into Down, Up, and Garbage
                            {
                              //sort for possible muon decay
			      downFlag=0;upFlag=0;noiseFlag=0;
                              if (code == 0x53)
                                {
                                  if (muhit == 0x1) upFlag=1;
                                  if (muhit == 0x2) downFlag=1;
                                  if (muhit == 0x4) noiseFlag=1;
                                }
                              if (code == 0x55)
                                {
                                  if (muhit == 0x1) upFlag=1;
				  if (muhit == 0x2) noiseFlag=1;
				  if (muhit == 0x4) downFlag=1;
                                }
                              if (code == 0x56)
                                {
                                  if (muhit == 0x1) noiseFlag=1;
				  if (muhit == 0x2) upFlag=1;
                                  if (muhit == 0x4) downFlag=1;
                                }
                              if (code == 0x57)
                                {
                                  if (muhit == 0x1) upFlag=1;
                                  if (muhit == 0x2) noiseFlag=1;
                                  if (muhit == 0x4) downFlag=1;
                                }
			      //
			      if (theConfiguration->writeEachHit)
				{
				  if (upFlag) fprintf(upward, "%s", datasend);
				  if (downFlag) fprintf(downward, "%s", datasend);
				  if (noiseFlag) fprintf(noise, "%s", datasend);
				}
			      //
			      // now want to bin data into histograms
			      //  for now histograms are fixed bins of mutime
			      //  (10 clock ticks) and raw mutime
			      //
			      histBin = mutime;
			      if (histBin < 0) histBin=0;
			      if (histBin >= maxBin ) histBin=maxBin-1;
			      coarseBin = mutime/coarseWidth;
			      if (upFlag)
				{
				  upHist[histBin]++;
				  upCoarse[coarseBin]++;
				}
			      if (downFlag)
				{
				  downHist[histBin]++;
				  downCoarse[coarseBin]++;
				}
			      if (noiseFlag)
				{
				  noiseHist[histBin]++;
				  noiseCoarse[coarseBin]++;
				}

                            }//end muon if


                          /* calculate rates per unit time interval */

                          if (theConfiguration->rateSummary>0)
                            {
                              rateInterval = ((int) totaltime)/
                                theConfiguration->rateSummary;
                              if (rateInterval>previousRateInterval)
                                {
                                  for(theInterval=previousRateInterval;
                                      theInterval<rateInterval;
                                      theInterval++)
                                    {
                                      flInterval = (float) nInterval;
                                      theRate = flInterval/
                                        ((float)theConfiguration->rateSummary);
                                      rateSigma = theRate/
                                        sqrt(flInterval);
                                      endTime = time(0);
                                      strftime(formattedDate,
                                               lengthFormattedDate,
                                               "%D\t%T",localtime(&endTime));
				      // header line
                                      if ( theInterval == 0 ) {
                                        sprintf(datasend,"Interval\tEnd Date\tEnd Time\tCounts\tRate(Hz)\tSigma Rate(Hz)");
					if (theConfiguration->efficient)
					  {
					    char tmpMask[12];
					    for (i=1;i<16;i++)
					      {
						sprintf(datasend2,"\t#%s",decodeMask(tmpMask,i));
						strcat(datasend,datasend2);
					      }
					  }
					sprintf(datasend2,"\n");
					strcat(datasend,datasend2);
                                        if (theConfiguration->outputfile)
                                          fprintf(outfile, "%s", datasend);
                                      }
				      // form print out
                                      sprintf(datasend,
                                              "%8i\t%s\t%5i\t%f\t%f",
                                              theInterval,formattedDate,
                                              nInterval,theRate,rateSigma);
                                      if(theConfiguration->efficient){
                                        for(i = 1;i<16; i++)
                                          {
                                            deltaEff=efficiency[i]-previousEfficiency[i];
					    sprintf(datasend2,"\t%10i",deltaEff);
					    strcat(datasend,datasend2);
					    previousEfficiency[i]=efficiency[i];
                                          }
				      }
				      sprintf(datasend2,"\n");
				      strcat(datasend,datasend2);
				      // print to file
                                      if (theConfiguration->outputfile)
                                        fprintf(outfile, "%s", datasend);
                                      // print to screen
                                      printf("%s", datasend);
                                      nInterval = 0;
                                    }
                                  previousRateInterval = rateInterval;
                                }
                            }

                        } /* custom run or coincidence */
                    } else {
                      if (theConfiguration->debug && theConfiguration->outputfile)
                        fprintf(outfile, "Badchar found; string omitted\n");
                    } // if (!badchar)
                  }  // if (!badchar)

                  if (theConfiguration->debug)
                    printf("Recorded event #%i at time %l\n",
                           eventcounter, totaltime);


                  break;
                }
              default:
                {
                  // printf("Reader read message type %ld : '%s'\n", msg->mtype, msg->mtext);
                  /* slow down a little... */
                  /* sleep(1); */
                }

              } // end switch
            } // end while loop


            totalhits = 0;

            printf ("Run ended.\n");
            // Step 3:  Tell the board to stop reading data.
            serialSend(serial,"WC 0\r");
            stoptime=time(0);
            if (theConfiguration->outputfile)
              fprintf(outfile,"Run Stopped at %s\n",ctime(&stoptime));

            // Step 3c:  End the time sender
            msg->mtype = END;
            sprintf(msg->mtext, timestr);
            if (theConfiguration->debug)
              printf("Main:About to stop timer\n");
            rc = msgsnd(time_queue_id, msg, strlen(msg->mtext)+1, 0);
            if (theConfiguration->debug)
              printf("Main:Timer message sent.\n");

            if (rc == -1) {
              perror("main: msgsnd");
              printf("Errno is %i\n", errno);
              done = 1;
            }

	    // print "efficiency" (hit type) summaries

            if(theConfiguration->efficient){
	      char tmpMask[12];
	      if (theConfiguration->rateSummary>0)
		{
		  sprintf(datasend,"\n%s\n","Sum over all intervals");
		  printf("%s",datasend);
		  if (theConfiguration->outputfile)
		    fprintf(outfile, "%s", datasend);
		  for(i = 1;i<16;i++)
		    {
		      sprintf(datasend,"#%s = %i\n",
			      decodeMask(tmpMask,i),previousEfficiency[i]);
		      printf("%s",datasend);
		      if (theConfiguration->outputfile)
			fprintf(outfile, "%s", datasend);
		    }
		}
	      sprintf(datasend,"\n%s\n","Sum over entire run");
	      printf("%s",datasend);
	      if (theConfiguration->outputfile)
		fprintf(outfile, "%s", datasend);
              for(i = 1;i<16;i++)
                {
		  sprintf(datasend,"#%s = %i\n",
			  decodeMask(tmpMask,i),efficiency[i]);
		  printf("%s",datasend);
		  if (theConfiguration->outputfile)
		    fprintf(outfile, "%s", datasend);
		  totalhits+=efficiency[i];
                  //reset the efficiency counter
                  efficiency[i] = 0;
                  previousEfficiency[i] = 0;
                }
              //printf("Sum of all types is %d\n",totalhits);
            }

	    // print muon lifetime histograms

	    if (theConfiguration->muon){
	      //
	      // print Fine histogram
	      //
	      sprintf(datasend,"\nMuon Lifetime Fine Histogram (clock ticks)\n        Time         \tHits\n");
              fprintf(upward, "%s", datasend);
              fprintf(downward, "%s", datasend);
              fprintf(noise, "%s", datasend);
	      for(i=0; i<maxBin; i++)
		{
		  fprintf(upward,"  %1.12f\t%i\n",i*muinterval,upHist[i]);
		  fprintf(downward,"  %1.12f\t%i\n",i*muinterval,downHist[i]);
		  fprintf(noise,"  %1.12f\t%i\n",i*muinterval,noiseHist[i]);
		  upHist[i]=0;downHist[i]=0;noiseHist[i]=0;
		}
	      //
	      // print Coarse histogram
	      //
	      sprintf(datasend,"\nMuon Lifetime Coarse Histogram (%i clock ticks/bin)\n       Time        \tHits\n",
		      coarseWidth);
              fprintf(upward, "%s", datasend);
              fprintf(downward, "%s", datasend);
              fprintf(noise, "%s", datasend);
	      for(i=0; i<maxCoarse; i++)
		{
		  fprintf(upward,"  %1.12f\t%i\n",i*coarseWidth*muinterval,upCoarse[i]);
		  fprintf(downward,"  %1.12f\t%i\n",i*coarseWidth*muinterval,downCoarse[i]);
		  fprintf(noise,"  %1.12f\t%i\n",i*coarseWidth*muinterval,noiseCoarse[i]);
		  upCoarse[i]=0;downCoarse[i]=0;noiseCoarse[i]=0;
		}
	      //
	    }

	    // final printout, close files

            sprintf(datasend, "Run complete. %i events in %i seconds.\n",
                    eventcounter, (int) (stoptime-starttime));

            if (theConfiguration->outputfile){
              fprintf(outfile, "%s\n", datasend);
              fflush(outfile);
              fclose(outfile);
              sprintf(filename,"timestamp"); // restore file name to default
            }
            if (theConfiguration->muon){
              fflush(noise);
              fflush(downward);
              fflush(upward);
              fclose(noise);
              fclose(downward);
              fclose(upward);
            }

            printf("%s",datasend);



            break;  // out of case: begin run
          }

          /**************************************/

        case '6':   // Change file name

          {
            char* token;
            char newFilename[90];
            char newFilestr[120];
            char overwrite;
	    int loop;
	    struct stat newStatus;

            printf ("\nName for data file (no spaces)\n");
            printf("(default is to base name on time):\n");

            receiveKeyString(newFilename);
            // strip everything after first space, CR or newline
            token=strtok(newFilename," \n\r");

            if (strlen(newFilename)==0 || strstr(" \n\r",newFilename)!=NULL
                || token == NULL )
              sprintf(filename,"timestamp");
            else
              {
                sprintf(newFilestr,"%s/muon.%s",getenv("HOME"),newFilename);
                printf("Testing new file %s\n",newFilestr);
                if (stat(newFilestr,&newStatus)!=-1)
                  {
		    printf("This file already exists. \n");
	       	    loop=0;
		    while (loop!=1)
		    {
			printf("Overwrite? y/n \n");
			overwrite=receiveKeyChar();
	       		if (overwrite=='n' || overwrite=='N')
			{
			    printf("File name unchanged.\n");
		            loop=1;
		        }
			if (overwrite=='y' || overwrite=='Y')
			{
			  sprintf(filename,"%s",newFilename);
			  loop=1;
		        }
		     }
		  }
                else
                  sprintf(filename,"%s",newFilename);
              }
            break;
          }


          /**************************************/

        case '7':
          {
            InterfaceConfigMenu(theConfiguration, serial);
            break;
          }

        case '8':  // barometer, thermometer, etc
          {
            float reading;

            if (theConfiguration->version != 2) break;

            // barometer... note force output not to go to logfile
	    if (theConfiguration->barometerCalibrated)
	      printf("Current barometer reading is %.2f kPa.\n", readBarometer(serial, main_queue_id, theConfiguration->barometerCalibrated, 0 , serial));
	    else
	      printf("Current barometer reading is %.0f (raw barometer counts)\n", readBarometer(serial, main_queue_id, theConfiguration->barometerCalibrated, 0, serial));


            // thermometer
            reading = readThermometer(serial, main_queue_id);
            if (reading > 200)
              printf("Nonsensical data collected from thermometer... check connection.\n");
            else printf("Current thermometer reading is %f C/%f F.\n", reading, (reading*9/5) + 32);

            // GPS
            if (readGPS(serial, main_queue_id, NULL) == 0)
              printf("GPS may not be operating properly...\nCheck connection and make sure antenna has ample view of sky.\n");

            break;
          }
        case 'q':
          {
            printf("Goodbye!\n");
            break;
          }

          /**************************************/

        default:
          {
            printf("Invalid choice.\n");
          }
        }
    } while(choicechar != 'q');

  // Close queues.
  if ( msgctl(main_queue_id, IPC_RMID, &descr) != 0 ) {
    perror("main: msgctl");
    exit(1);
  }

  if ( msgctl(time_queue_id, IPC_RMID, &descr) != 0 ) {
    perror("Time: msgctl");
    exit(2);
  }

  // deallocate memory
  free(msg);
  fclose(serial);
  return 0;
}



/*************************************************************/


void CRContents(unsigned char regis)
{
  char coincidence;
  char inhibits;

  printf("Current channel status:\n");

  if ((regis&0x01)==0x01)  printf("\tChannel 1 is enabled\n");
  else  printf("\tChannel 1 is disabled\n");

  if ((regis&0x02)==0x02)  printf("\tChannel 2 is enabled\n");
  else  printf("\tChannel 2 is disabled\n");

  if ((regis&0x04)==0x04)  printf("\tChannel 3 is enabled\n");
  else  printf("\tChannel 3 is disabled\n");

  if ((regis&0x08)==0x08)  printf("\tChannel 4 is enabled\n");
  else  printf("\tChannel 4 is disabled\n");

  printf("\n");

  coincidence = ((regis & 0x30) >> 4) + 1;
  printf("The required coincidence level is %i\n",(int)coincidence);

  printf("\n");

  inhibits = (regis & 0xC0) >> 6;
  printf("The inhibit (VETO) level is %i\n",(int)inhibits);

  printf("\n");
}

void serialSend(FILE* fileStream,const char* theMessage)
{
#ifndef FAKESERIAL
  fprintf(fileStream,"%s",theMessage);
  fflush(fileStream);
  usleep(80000); // (20 characters at 9600 baud)*5
#endif
}

char* decodeMask(char* coderep, int code)
{
  /* want to translate this code into, e.g. (1,-,-,4) */
  sprintf(coderep,"(%1i,%1i,%1i,%1i)",
	  code&0x1,code&0x2,
	  (code&0x4)/4*3,(code&0x8)/2);

  while (strchr(coderep,'0') != NULL)
    *strchr(coderep,'0')='-';

  return(coderep);
}

/* Queries board counters (new board only) */
/* KSM, 12 September 2005 revision: some new board programs
   have a new format for this information!
   DS S0=xxxxxxxx S1=xxxxxxxx, etc.
   instead of multiple line format.  Want to support both */
void getCounts(int *c1, int *c2, int *c3, int *c4, int *trig,
	       FILE *serial, int qid)
{
  int rc, i, i1, i2, i3, i4, i5, i6, j, firstDig;
  char data[100], chstr[20], outstr[30];

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE,
      IPC_NOWAIT);

  serialSend(serial,"DS\r");
  usleep(100000);

  rc = 0;
  *c1 = 0;
  *c2 = 0;
  *c3 = 0;
  *c4 = 0;
  *trig = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
  {
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE,
      IPC_NOWAIT);
    sprintf(data, msg->mtext);
    if (data[0] == '@')
    {
      // printf ("Old style response: %s\n",data);
      for (i = 0; i < (strlen(data) - 4) && i < 19; i++)
        chstr[i] = data[i+4];
      chstr[i] = (char)NULL;
      sscanf(chstr, "%xl", &i);
      switch(data[2])
      {
        case '0':
          *c1 = i;
          break;
        case '1':
          *c2 = i;
          break;
        case '2':
          *c3 = i;
          break;
        case '3':
          *c4 = i;
          break;
        case '4':
          *trig = i;
          break;
        default:
          break;
      }
    } // old format
    else if (data[0]=='D' && data[3]=='S') {
      // printf ("New style response: %s\n",data);
      for (j = 0; j < 6; j++) {
	firstDig = 6+j*12;
	for (i = firstDig; i < strlen(data) && i < firstDig+8; i++)
	  chstr[i-firstDig] = data[i];
	{
	  chstr[i-firstDig] = (char)NULL;
	  sscanf(chstr, "%xl", &i);
	  switch(data[firstDig-2])
	    {
	    case '0':
	      *c1 = i;
	      break;
	    case '1':
	      *c2 = i;
	      break;
	    case '2':
	    *c3 = i;
	    break;
	    case '3':
	      *c4 = i;
	      break;
	    case '4':
	      *trig = i;
	      break;
	    default:
	      break;
	    }
	} // loop over characters within a hex string
      } // loop over outputs
      // printf ("I understood you to say the #s are: %8x, %8x, %8x, %8x, %8x\n", *c1, *c2, *c3, *c4, *trig);
    } // new format
    //else
      //printf ("Not understood response: %s\n",data);
  }
}

/* reads control register 0 and returns value (new board only) */
unsigned char getCRStatus(FILE *serial, int qid)
{

  int rc, temp, i, i1, i2, i3, i4, j, firstDig;
  unsigned char crdata;
  char data[100], chstr[20];

  serialSend(serial, "CD\r"); // mute the FIFO output

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE,
      IPC_NOWAIT);

  serialSend(serial,"DC\r");
  usleep(100000);

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
  {
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE,
      IPC_NOWAIT);
    sprintf(data, msg->mtext);
    if (data[0] == '@' && data[2] == '0')
    {
      for (i = 0; i < (strlen(data) - 4) && i < 6; i++)
        chstr[i] = data[i+4];
      chstr[i] = (char)NULL;
      sscanf(chstr, "%xl", &temp);
      crdata = (unsigned char)temp;
    } // old format
    else if (data[0]=='D' && data[3]=='C') {
      for (j = 0; j < 5; j++) {
	firstDig = 6+j*12;
	for (i = firstDig; i < strlen(data) && i < firstDig+8; i++)
	  chstr[i-firstDig] = data[i];
	{
	  chstr[i-firstDig] = (char)NULL;
	  sscanf(chstr, "%xl", &i);
	  switch(data[firstDig-2])
	    {
	    case '0':
	      crdata = (unsigned char) i;
	      break;
	    default:
	      break;
	    }
	} // loop over characters within a hex string
      } // loop over outputs
    } // new format
  }
  //serialSend(serial,"CE\r");   // reenable the FIFO output

  return crdata;
}

/* returns calibrated barometer reading (pressure in kPa) */
float readBarometer(FILE *serial, int main_queue_id, int isCalibrated, int rawBoard, FILE *outfile)
{
  int rc;
  int rawReading;
  float calibrated;
  float reading;
  char data[100];

  serialSend(serial,"CD\r");   // stop the FIFO output

  rc = 0;
  rawReading = 9999;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);

  serialSend(serial,"BA\r");

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
  {
    rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);
    sprintf(data, msg->mtext);
    if (rawBoard != 0) {
      printf ("%s\n",data);
      fprintf (outfile,"%s\n",data);
    }
    if (data[0] >= '0' && data[0] <= '9')
      sscanf(data, "%i %f", &rawReading, &calibrated);
    if (data[0] = 'B')
      sscanf(data, "BA %i %f", &rawReading, &calibrated);
  }

  serialSend(serial,"CE\r");   // enable the FIFO output

  if (isCalibrated)
    reading = calibrated;
  else
    reading = rawReading;

  return reading;
}

/* returns thermometer reading (from GPS antenna) */
float readThermometer(FILE *serial, int main_queue_id)
{
  int rc;
  float reading;
  char data[100];

  serialSend(serial,"CD\r");   // disable the FIFO output

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);

  serialSend(serial,"TH\r");

  rc = 0;
  reading = 999.0;
  while ( (errno != ENOMSG) || (rc != -1) )
  {
    rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);
    sprintf(data, msg->mtext);
    if (data[0] >= '0' && data[0] <= '9')
      sscanf(data, "%f", &reading);
    if (data[0] = 'T')
      sscanf(data, "TH %f", &reading);
  }

  serialSend(serial,"CE\r");   // reenable the FIFO output
  return reading;
}

/* gets GPS avg frequency for PPS */
int getGPSfrq(FILE *serial, int qid)
{
  int rc, i, j, avg;
  char data[100], avgStr[16];

  serialSend(serial, "CD\r");

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);

  serialSend(serial, "DG\r");
  usleep(200000);

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) ) {
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1, SERIAL_TYPE, IPC_NOWAIT);
    sprintf(data, msg->mtext);
    i = 0;
    while (data[i] == ' ' || data[i] == '>') i++;
    if (data[i] == 'C' && data[i+5] == 'f') {
      while (data[i] != '4') i++;
      for (j = 0; data[i] != ' '; i++, j++) avgStr[j] = data[i];
      avgStr[j] = (char)NULL;
      sscanf(avgStr, "%8d", & avg);
      break;
    }
  } // end while
  return avg;
}

/* queries the GPS */
int readGPS(FILE *serial, int qid, FILE *outfile)
{
  int rc, i, numSats, writeFile;
  char data[100], d1[16], d2[16], d3[16];

  if (outfile == NULL) writeFile = 0;
  else writeFile = 1;

  serialSend(serial,"CD\r");   // disable the FIFO output

  rc = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);

  serialSend(serial,"DG\r");
  usleep(200000);

  rc = 0;
  numSats = 0;
  while ( (errno != ENOMSG) || (rc != -1) )
  {
    rc = msgrcv(qid, msg, MAX_MSG_SIZE+1,
                SERIAL_TYPE, IPC_NOWAIT);
    sprintf(data, msg->mtext);
    i = 0;      // get past whitespace etc
    while(data[i] == ' ' || data[i] == '>') i++;
    switch(data[i])
    {
        case 'D':       // date and time
	  if (data[i+1] == 'G') break;  // DG command echo
          sscanf(data, "%s %s %s", &d1, &d2, &d3);
          printf("GPS Date: %s\nGPS Time: %s\n", d2, d3);
	  if (writeFile) fprintf(outfile, "GPS Date: %s\nGPS Time: %s\n", d2, d3);
          break;
        case 'L':      // latitude or longitude
          printf("%s", data);
	  if (writeFile) fprintf(outfile, "%s", data);
          break;
        case 'A':      // altitude
          printf("%s", data);
	  if (writeFile) fprintf(outfile, "%s", data);
          break;
      case 'S':
        if (data[i+1] == 'a')   // # satellites used
        {
          sscanf(data, "%s %s %i", &d1, &d2, &numSats);
          printf("%i satellites in use.\n", numSats);
	  if (writeFile) fprintf(outfile, "%i satellites in use.\n", numSats);
        }
        else if(data[i+1] == 't') // status of data
        {
          sscanf(data, "%s %s %s", &d1, &d2, &d3);
          if (d3[1] != 'v')    // invalid data
          {
            printf("Error: GPS data invalid.\n");
	    if (writeFile) fprintf(outfile, "Error: GPS data invalid.\n");
            serialSend(serial,"CE\r");   // reenable the FIFO output
            return 0;
          }
        }
        break;
      default: break;
    }
  }

  serialSend(serial,"CE\r");   // reenable the FIFO output
  return numSats;
}
