// MENU.CPP - Main interface module for muon telescope
//
//            VERSION 1.0 (7.12.00)
//

// INCLUDES ///////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <sys/types.h>   /* various type definitions.            */
#include <asm/errno.h>   /* access to error code, e.g. EIDRM     */
#include "quarknet.h"
#include "queue_defs.h"
#include "configuration.h"


char* receiveKeyString(char* str)
{
  int rc,msgtype;
  struct msgbuf* msg;       /* structure used for received messages. */
  int main_queue_id;        /* ID of the created queue.              */
  
  msg = (struct msgbuf*)malloc(sizeof(struct msgbuf)+MAX_MSG_SIZE);

  /* access the MAIN message queue that the reader program created. */
  main_queue_id = msgget(MAIN_QUEUE_ID, 0);
  if (main_queue_id == -1) {
    perror("Key: msgget");
    exit(1);
  }
  
  /* blocking receive on input queue */

  /* used to require KEY_TYPE...
     flaw is that a filled queue results in a hung program */
  //rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, KEY_TYPE, 0);

  msgtype = -1;
  while (msgtype != KEY_TYPE)
    {
      rc = msgrcv(main_queue_id, msg, MAX_MSG_SIZE+1, 0, 0);
      if (rc == -1) {
        perror("main: msgrcv");
        exit(1);
      }
      msgtype = msg->mtype;
    }

  sprintf(str, msg->mtext);

  free (msg);

  return str;
}


char receiveKeyChar(void) 
{
  char ch;
  char str[100];
  receiveKeyString(str);
  ch = (char) str[0];
  return ch;
}


char mainMenu(char* file, interfaceConfiguration* theConfiguration)
{
  char choicechar;

   // Write out the main menu
   static char rcsdate[] = "$ Date: 2003/08/05 $";
   printf("\fMuon Telescope DAQ\t\tDigamma Version\t\t%s",rcsdate);
   printf("\n\n    Main Menu\n\n");
   printf("(1) Set control register (channels enables, coincidence, veto)\n");
   printf("(2) Display the contents of the control register\n");
   printf("(3) Measure Single Counter Rates\n");
   printf("(4) Begin Coincidence Run (all channels, 2 required, no veto)\n");
   printf("(5) Begin Custom (use control register) \n");
   if (theConfiguration->outputfile || theConfiguration->rawoutputfile)
     {
       printf("(6) Modify data file name (currently ");
       if (strcmp(file,"timestamp")==0)
         printf("based on time)\n");
       else
         printf("'%s')\n",file);
     }
   printf("(7) Configure this program\n");
   if (theConfiguration->version == 2)
     printf("(8) Display external board information\n");
   printf("(q) Quit\n");
   printf("\n");
   printf("What do you want to do? \n");
   choicechar = receiveKeyChar();

   return choicechar;
}


unsigned char CRWriteMenu(unsigned char regis)
{
   char regbit;                        // Register Change

	  int loop;
	  loop = 1;
	  do {
	  printf("\nWhat do you want to do? \n");
	  printf("(a) Enable/Disable inputs. \n");
          printf("(b) Set the coincidence level. \n");
	  printf("(c) Select a channel for VETO. \n");
	  printf("(x) Return to main menu. \n");

	  regbit = receiveKeyChar();

	  switch(regbit) {
	  case 'A':
	  case 'a': 
	    {
	     char ch1on, ch2on, ch3on, ch4on;    // Channel selectors

	      // print out state of inputs
	      if ( (regis & 0x01) == (0x01)) {
		printf("Channel 1 enabled \n");
	      } else {
		printf("Channel 1 is disabled \n");
	      }

	      if ( (regis & 0x02) == (0x02)) {
		printf("Channel 2 is enabled \n");
	      } else {
		printf("Channel 2 is disabled \n");
	      }

	      if ( (regis & 0x04) == (0x04)) {
		printf("Channel 3 is enabled \n");
	      } else {
		printf("Channel 3 is disabled \n");
	      }

	      if ( (regis & 0x08) == (0x08)) {
		printf("Channel 4 is enabled \n");
	      } else {
		printf("Channel 4 is disabled \n");
	      }

	      printf("Do you want to enable channel 1?\n");
	      ch1on = receiveKeyChar();

	      printf("Do you want to enable channel 2?\n");
	      ch2on = receiveKeyChar();

	      printf("Do you want to enable channel 3?\n");
	      ch3on = receiveKeyChar();

	      printf("Do you want to enable channel 4?\n");
	      ch4on = receiveKeyChar();

	      // Clear the input bits (bits 0,1,2,3)
	      regis = regis & 0xF0;

	      // Reset the input bits.
	      if ((ch1on == 'y') || (ch1on == 'Y')) {
		regis = regis | 0x01;
	      } else if (ch1on != 'n' && ch1on != 'N') {
		printf("Error! Invalid entry in channel 1! \n");
	      }

	      if ((ch2on == 'y') || (ch2on == 'Y')) {
		regis = regis | 0x02;
	      } else if (ch2on != 'n' && ch2on != 'N') {
		printf("Error! Invalid entry in channel 2! \n");
	      }

	      if ((ch3on == 'y') || (ch3on == 'Y')) {
		regis = regis | 0x04;
	      } else if (ch3on != 'n' && ch3on != 'N') {
		printf("Error! Invalid entry in channel 3! \n");
	      }

	      if ((ch4on == 'y') || (ch4on == 'Y')) {
		regis = regis | 0x08;
	      } else if (ch4on != 'n' && ch4on != 'N') {
		printf("Error! Invalid entry in channel 4! \n");
	      }

	      printf("The new configuration of the channels is: \n");

	      if ( (regis & 0x01) == (0x01)) {
		printf("Channel 1 enabled \n");
	      } else {
		printf("Channel 1 is disabled \n");
	      }

	      if ( (regis & 0x02) == (0x02)) {
		printf("Channel 2 is enabled \n");
	      } else {
		printf("Channel 2 is disabled \n");
	      }

	      if ( (regis & 0x04) == (0x04)) {
		printf("Channel 3 is enabled \n");
	      } else {
		printf("Channel 3 is disabled \n");
	      }

	      if ( (regis & 0x08) == (0x08)) {
		printf("Channel 4 is enabled \n");
	      } else {
		printf("Channel 4 is disabled \n");
	      }

	      break; // out of switch for channels
	    }
	  // probe for number of coincidences 
	  case 'B':
	  case 'b':
	   {
	     char coin;

	    printf("Enter the number of coincidences desired. \n");
	    printf("Possible values are 1,2,3,4.\n");
	    coin = receiveKeyChar();
	    if (coin > 0x34 || coin < 0x31) {
	      printf("Error!  Invalid entry!");
	    } else {
	      coin -= 0x31;  // ASCII code for '1'; possible values:  0,1,2,3
	      coin = coin << 4;  // xx becomes 00xx0000
	      regis = regis & 0xCF; // 11001111
	      regis = regis | coin;
	    }
	    // State the new coincidence level
	    printf("The new coincidence level is %i\n conincidences",
		   (int)((regis>>4)&(0x03))+1);

	    break; // out of coincidence switch
	   }
	  // probe inhibit select
	  case 'C':
	  case 'c':
	   {
	    char inhibit,paddle;
	    printf("What VETO configuration do you want? \n");
	    printf("Possible values:  0,1,2,3 \n");
	    printf("0 is no VETO; 1, 2, or 3 will put that channel in VETO.\n");
	    paddle = receiveKeyChar();
	    if (paddle > 0x33 || paddle < 0x30) {
	      printf("Error!  Invalid entry!");
	    } else {
	      paddle -= 0x30;
	      inhibit = paddle << 6;  // xx becomes xx000000
	      regis = regis & 0x3F; // 00111111
	      regis = regis | inhibit;
	    }
	    // State the new veto level
        if (paddle == 0)
          printf("No VETO selected\n");
        else
          printf("Channel %i is in VETO.\n",paddle);
	    break; // out of inhibit switch
	   }
	    // Back to main menu
	  case 'X':
	  case 'x':
	    loop=0;
	    break;

	  default:
	    printf("Error! Invalid entry!\n");
	    break;
	  } // end switch(regbit)
	  } while (loop);
   return regis;
}

void printLogical(int logicalInput)
{
  if (logicalInput)
    printf ("TRUE");
  else
    printf ("FALSE");
}

void InterfaceConfigMenu(interfaceConfiguration* theConfiguration, FILE
*serial)
{
  char theInput;                        // input character
  char response;                        // question response
  char* token;
  char summaryInterval[10];
  char newSerial[SERIAL_LENGTH];

  int loop = 1; // control loop
  do {
    printf("\nCurrent configuration:\n");
    printf("(a) BEEP each event (currently ");
    printLogical(theConfiguration->beep);
    printf(")\n");
    printf("(b) Write to an OUTPUT FILE (currently ");
    printLogical(theConfiguration->outputfile);
    printf(")\n");
    printf("(c) Write raw data to an OUTPUT FILE (currently ");
    printLogical(theConfiguration->rawoutputfile);
    printf(")\n");
    printf("(d) Information to write (currently ");
    if (theConfiguration->writeEachHit && theConfiguration->rateSummary <= 0)
      printf("every hit");
    else if (theConfiguration->writeEachHit)
      printf("every hit, ");
    if (theConfiguration->rateSummary > 0)
      printf("rates every %i seconds", theConfiguration->rateSummary);
    else if (!theConfiguration->writeEachHit)
      printf("no singles, no summary");
    printf(")\n");
        /*modified for muon lifetime run--------------*/
    printf("(e) Configure for Muon Lifetime Run (currently ");
    if (theConfiguration->muon == 0) printf("disabled");
    else if (theConfiguration->muon == 1) printf("enabled: no absorber");
    else printf("enabled: absorber below paddle %i", theConfiguration->muon - 1);
    printf(")\n");
    printf("(f) Report summary of hit types, as for efficiency runs (currently ");
    printLogical(theConfiguration->efficient);
    printf(")\n");
    printf("(g) Report paddles hit as hexidecimal code (currently ");
    printLogical(theConfiguration->hexCode);
    printf(")\n");
    printf("(h) Set time in seconds for singles run (Currently %i)\n",theConfiguration->runlen);
    printf("(i) Output raw board data from FIFO buffer (currently ");
    printLogical(theConfiguration->rawBoard);
    printf(")\n");
    if (theConfiguration->version == 2)
    {
      printf("(j) Report rising and falling edge times (currently ");
      printLogical(theConfiguration->riseFallTimes);
      printf(")\n");
      printf("(k) Extra trigger gate width (currently ");
      printf("%i",theConfiguration->triggerWidth);
      printf(" ns)\n");
      printf("(l) Muon speed mode (currently ");
      if (theConfiguration->muonSpeed > 1 && (theConfiguration->muonSpeed&512)==0)
      {
        printf("on: channel ");
        switch ((theConfiguration->muonSpeed&30)>>1)
        {
          case 1: printf("1, "); break;
          case 2: printf("2, "); break;
          case 4: printf("3, "); break;
          case 8: printf("4, "); break;
        }
        switch ((theConfiguration->muonSpeed&480)>>5)
        {
          case 1: printf("1)"); break;
          case 2: printf("2)"); break;
          case 4: printf("3)"); break;
          case 8: printf("4)"); break;
        }
        printf("\n");
      }
      else if ((theConfiguration->muonSpeed&512) != 0) printf("on: selected at run time)\n");
      else if (theConfiguration->muonSpeed == 1) printf("on: any two channels)\n");
      else printf("off)\n");
      if (theConfiguration->rateSummary > 0)
      {
        printf("(m) Report temperature and pressure for rate summaries (currently ");
        printLogical(theConfiguration->rateSummaryGoodies);
        printf(")\n");
      }
      printf("(n) Calibrate barometer\n");
    }
    printf("(o) Display GPS times (currently ");
    printLogical(theConfiguration->useGPS);
    if (theConfiguration->useGPS) {
      if (theConfiguration->reportSats)
	printf(", sats on");
      else
	printf(", no sats");
      if (theConfiguration->useAvgFreq)
	printf(", avg. %d PPS",theConfiguration->avgAmount);
      else
	printf(", no PPS avg.");
    }
    printf(")\n");
    printf("(p) Fix GPS times to computer time zone (currently ");
    printLogical(theConfiguration->timeFix);
    printf(")\n");
    printf("\nExpert only:\n");
    printf("(t) Print lots of output for debugging (currently ");
    printLogical(theConfiguration->debug);
    printf(")\n");
    printf("(u) Serial device name (currently %s)\n",theConfiguration->serialDevice);
    printf("(v) DAQ board version (currently ");
    switch (theConfiguration->version)
    {
      case 0: printf("old)\n"); break;
      case 1: printf("old revised)\n"); break;
      case 2: printf("new)\n"); break;
      default: printf("unknown!)\n"); break;
    }
    if (theConfiguration->version == 2)
    {
    }
    printf("(w) Only display raw GPS second (currently ");
    printLogical(theConfiguration->rawGPSdata);
    printf(")\n");
    printf("\n(x) return to main menu\n\n");
    printf("What do you want to do?\n");

    theInput = receiveKeyChar();

	  switch(theInput) {
	  
	  case 'A':
	  case 'a': 
	    {
	      theConfiguration->beep = 1-theConfiguration->beep; //toggle
	      break;
	    }

	  case 'B':
	  case 'b': 
	    {
	      theConfiguration->outputfile = 1-theConfiguration->outputfile; //toggle
	      break;
	    }
	  case 'C':
	  case 'c':
	    {
	      theConfiguration->rawoutputfile = 1-theConfiguration->rawoutputfile;
	      break;
	    }
	  case 'D':
	  case 'd': 
	    {
	      printf("Do you want to log every event?\n");
	      response = receiveKeyChar();
	      theConfiguration->writeEachHit =  
		( response == 'Y' || response == 'y' );
	      printf("Do you want to report rate summaries?\n");
	      response = receiveKeyChar();
	      if ( response == 'Y' || response == 'y' )
		{
		  printf("    Enter reporting interval in seconds:\n");
		  receiveKeyString(summaryInterval);
		  token=strtok(summaryInterval," \n\r");
		  if ((strlen(summaryInterval)==0 
		       || strstr(" \n\r",summaryInterval)!=NULL
		       || token == NULL ))
		    {
		      printf ("  INVALID RESPONSE, rate summaries turned off\n");
		      theConfiguration->rateSummary = 0;
		    }
		  else
		    {
		      sscanf(summaryInterval, "%i", 
			     &(theConfiguration->rateSummary));
		      printf ("  rate summary frequency is %i seconds.\n",
			      theConfiguration->rateSummary);
		    }
		}
	      else
		theConfiguration->rateSummary = 0;
	      break;
	    }

	    /* Muon Lifetime-----------------------*/
	  case 'E':
	  case 'e':
	    {
              char strin[20];
              int datain;

              if (theConfiguration->version == 2)
              {
                if (theConfiguration->muon)
                {
                  theConfiguration->muon = 0;
                  break;
                }
                printf("Enter the paddle (1-4) below which the absorber lies.\n");
                printf("Enter '0' if you are not using an absorber.\n");
                receiveKeyString(strin);
                sscanf(strin, "%i", &datain);
                if (datain < 0 || datain > 4) datain = 0;
                theConfiguration->muon = datain+1;
                break;
              }
	      theConfiguration->muon = 1-theConfiguration->muon;//toggle
	      printf("This assumes the following paddle setup when true:\n");
	      printf("channel 1 ---------- 01, top paddle\n");
	      printf("channel 2 ---------- 02, middle paddle\n");
	      printf("channel 3 ---------- 03, bottom paddle\n");
	      printf("This code will only work with a 2 or 3 paddle set up, and only in this configuration!!!\n");
	      break;
	    }
	    /*-------------------------------------*/
	  case 'F':
	  case 'f':
	    {
	      theConfiguration->efficient =1-theConfiguration->efficient;//toggle
	      break;
	    }

	  case 'G':
	  case 'g':
	    {
	      theConfiguration->hexCode =1-theConfiguration->hexCode;//toggle
	      break;
	    }

	  case 'H':
	  case 'h': 
	    {
              int runl;
              char strin[20];

              printf("How many seconds would you like the run to be?\n");
              printf("Enter a number from 1 to 60: ");
              fflush(stdout);
              receiveKeyString(strin);
              sscanf(strin, "%i", &runl);
              if (runl > 60 || runl < 0)
              {
                printf("Bad run length!  Using 10 second run instead.\n");
                runl = 10;
              }
              theConfiguration->runlen = runl;
              break;
            }

	  case 'I':
	  case 'i': 
	    {
	      theConfiguration->rawBoard = 1-theConfiguration->rawBoard;
	      break;
	    }

	  case 'J':
	  case 'j': 
	    {
	      theConfiguration->riseFallTimes = 1-theConfiguration->riseFallTimes;
	      break;
	    }

	  case 'K':
	  case 'k': 
	    {
/*               int runl; */
/*               char strin[20]; */

/*               printf("How many nanoseconds would you like to add to the ~50 ns trigger gate?\n"); */
/*               printf("Enter a number between 0 and 2000: "); */
/*               fflush(stdout); */
/*               receiveKeyString(strin); */
/*               sscanf(strin, "%i", &runl); */
/*               if (runl > 2000 || runl < 0) */
/*               { */
/*                 printf("Bad trigger width!  Using default instead.\n"); */
/*                 runl = 10; */
/*               } */
/*               theConfiguration->triggerWidth = runl; */

	      if (theConfiguration->triggerWidth == 0 ) {
		printf("Adding 1 microsecond to gate width\n");
		theConfiguration->triggerWidth = 1000;
	      }
	      else {
		printf("Resetting to default (~50 ns) trigger width");
		theConfiguration->triggerWidth = 0;
	      }
              break;
            }

	  case 'L':
	  case 'l': 
	    {
              char strin[15];
              int indata, first, second;

              if (theConfiguration->version != 2) break;
              printf("Do you want to measure time of flight between two specific paddles?\n");
              receiveKeyString(strin);
              if (strin[0] == 'y' || strin[0] == 'Y')
              {
                printf("Choose the first paddle (enter channel number  1, 2, 3, or 4):\n");
                receiveKeyString(strin);
                sscanf(strin, "%i", &indata);
                if (indata < 1 || indata > 4)
                {
                  printf("Invalid choice for channel.\n");
                  break;
                }
                first = indata;

                printf("Choose the second paddle (enter channel number  1, 2, 3, or 4):\n");
                receiveKeyString(strin);
                sscanf(strin, "%i", &indata);
                if (indata < 1 || indata > 4)
                {
                  printf("Invalid choice for channel.\n");
                  break;
                }
                second = indata;

                theConfiguration->muonSpeed = (1<<first)+(16<<second);
                break;
              }

              printf("Do you want to select paddles for speed mode at run time?\n");
              receiveKeyString(strin);
              if (strin[0] == 'y' || strin[0] == 'Y')
              {
                printf("This mode will prompt for paddles at run time\n");
                theConfiguration->muonSpeed = 512;
                break;
              }
              printf("Do you want to measure time of flight between any two paddles?\n");
              receiveKeyString(strin);
              if (strin[0] == 'y' || strin[0] == 'Y')
              {
                printf("Time of flight between any two paddles requires exactly two paddles be hit in each event.\n");
                theConfiguration->muonSpeed = 1;
                break;
              }

              printf("Time of flight mode deactivated.\n");
              theConfiguration->muonSpeed = 0;
            }
            break;

	  case 'M':
	  case 'm': 
	    {
              if (!(theConfiguration->version == 2 && theConfiguration->rateSummary>0))
                break;
              theConfiguration->rateSummaryGoodies = 1-theConfiguration->rateSummaryGoodies;
            }
            break;

	  case 'N':
	  case 'n': 
	    {
              char strin[20];
              float indata;

              if (theConfiguration->version != 2) break;

              printf("Barometer calibration is of the form:\n");
              printf("Pressure (in kPa) = Baseline + (RawData / Gain)\n");

              printf("Enter floating point value for Baseline: ");
              fflush(stdout);
              receiveKeyString(strin);
              sscanf(strin, "%f", &indata);

              // error checking to be done here
              theConfiguration->barA = indata;

              printf("Enter floating point value for Gain: ");
              fflush(stdout);
              receiveKeyString(strin);
              sscanf(strin, "%f", &indata);

              // error checking to be done here
              theConfiguration->barB = indata;

              // update board with new calibration information
              sprintf(strin, "BC %.4f %.4f\r", theConfiguration->barA, theConfiguration->barB);
              serialSend(serial, strin);

	      theConfiguration->barometerCalibrated = TRUE;

              break;
            }
	  case 'O':
	  case 'o':
	    {
	      char cin;
	      theConfiguration->useGPS = 1-theConfiguration->useGPS;
	      while(theConfiguration->useGPS) {
	    	printf("Would you like to display satellite info? (y/n)\n");
        	cin = receiveKeyChar();
            	if (cin == 'Y' || cin == 'y') {
          	  theConfiguration->reportSats = 1;
          	  printf("Satellite info will be displayed.\n\n");
		  break;
          	}
            	else if (cin == 'N' || cin == 'n') {
          	  theConfiguration->reportSats = 0;
          	  printf("Satellite info will not be displayed.\n\n");
		  break;
          	}
	        else printf("   INVALID RESPONSE\n");	
              }
              while(theConfiguration->useGPS) {
		printf("Would you like to use the average frequency? (y/n)\n");
		cin = receiveKeyChar();
	        if (cin == 'Y' || cin == 'y') {
		  theConfiguration->useAvgFreq = 1;
		  printf("The average frequency will be used in calculation.\n\n");
		  printf("How many numbers should be used to calculate the average? (max = 50)\n");
		  receiveKeyString(summaryInterval);
		  token = strtok(summaryInterval, " \n\r");
		  if ((strlen(summaryInterval) == 0 ||
		       strstr(" \n\r", summaryInterval) != NULL ||
		       token == NULL))
			 printf("   INVALID RESPONSE\n");
		  else
		    sscanf(summaryInterval, "%i", & (theConfiguration->avgAmount) );
		  if (theConfiguration->avgAmount>50) theConfiguration->avgAmount = 50;
		  if (theConfiguration->avgAmount<1) theConfiguration->avgAmount = 1;
		  printf("%i numbers will be used in the calculation of frequency.\n\n",
			theConfiguration->avgAmount);
		  break;
	        }
		else if (cin == 'N' || cin == 'n') {
		  theConfiguration->useAvgFreq = 0;
		  printf("The frequency will be calculated based on previous data.\n\n");
		  break;
		}
		else printf("   INVALID RESPONSE\n");
	      }
	      break;
	    }
	  case 'P':
	  case 'p':
	    {
	      theConfiguration->timeFix = 1-theConfiguration->timeFix;
	      break;
	    }
	  case 'T':
	  case 't': 
	    {
	      theConfiguration->debug = 1-theConfiguration->debug; //toggle
	      break;
	    }

	  case 'U':
	  case 'u': 
	    {
	      printf("\n   Dynamic choice of serial port not supported.\n");
	      printf("   Change %s and restart.\n",PORT_FILE);
	      // receiveKeyString(newSerial);
	      // // strip everything after first space, CR or newline
	      // token=strtok(newSerial," \n\r"); 
	      // if (strcmp(newSerial,"/dev/ttyS1") && strcmp(newSerial,"/dev/ttyS0"))
	      // printf("   Serial device name must be /dev/ttyS0 or /dev/ttyS1\n");
	      // else
	      // strcpy(theConfiguration->serialDevice,newSerial);
	      break;
	    }

	  case 'V':
	  case 'v': 
	    {
              char ch;
              int ver;

              printf("\nChoose DAQ board version:\n\n");
              printf("(a) Old board\n(b) Revised old board\n(c) New Board.\n\n");
              ch = receiveKeyChar();
              switch(ch)
              {
                case 'a': case 'A':
                  printf("Old board chosen.\n");
                  ver = 0;
                  break;
                case 'b': case 'B':
                  printf("Old board revised version chosen.\n");
                  ver = 1;
                  break;
                case 'c': case 'C':
                  printf("New board chosen.\n");
                  ver = 2;
                  break;
                default:
                  printf("Invalid choice.\n");
                  ver = theConfiguration->version;
              }
	      theConfiguration->version = ver;
	      break;
	    }
	  case 'W':
	  case 'w':
	    {
	      if (theConfiguration->useGPS)
		theConfiguration->rawGPSdata = 1-theConfiguration->rawGPSdata;
	      else printf("GPS times must be used!\n");
	      break;
	    }
	  case 'X':
	  case 'x': 
	    {
	      loop = 0; // exit
	      break;
	    }
	  }
  } while (loop);
  return;
}




