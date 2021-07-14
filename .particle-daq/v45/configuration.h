#ifndef CONFIGURATION_H
# define CONFIGURATION_H

/*
 * configuration.h - parameters that define the interface configuration
 */

#ifndef   TRUE
#define   TRUE          (1)
#endif

#ifndef   FALSE
#define   FALSE         (0)
#endif

#define SERIAL_LENGTH (12)

char portstr[80];
#define PORT_FILE strcat(strcpy(portstr,getenv("HOME")),"/.particle-port")

typedef struct
          { int beep;                /* beep each event */
            int debug;               /* verbose debug output */
            int outputfile;          /* write an output file */
	    int rawoutputfile;	     /* write an output file with raw data */
	    int useGPS;		     /* displays GPS time */
	    int rawGPSdata;	     /* only displays GPS second calculated, used
					for running extended tests */
	    int reportSats;          /* displays the number of satellites connected */
	    int timeFix;             /* fixes time zone */
	    int useAvgFreq;	     /* uses avg freq for GPS */
	    int avgAmount;	     /* number of data to avg */
            int version;             /* DAQ board version (0,1,2) */
            int writeEachHit;        /* log each hit to output file */
            int rateSummary;         /* log a rate summary over this 
                                        number of seconds (<=0 is off) */
	    int muon;                /* Uses code for muon lifetime run */
	    int efficient;           /* Allows print out of hit summary*/
	    int hexCode;             /* report paddle masks as hex code */
            int runlen;              /* singles run length in seconds */
            int rateSummaryGoodies;  /* temp+pressure in rate sums? */
            int muonSpeed;           /* muon speed mode... see next line */
/* 1s bit is "any paddles", 
   2s-16s and 32s-256s bits are start and stop paddle masks, 
   512s bit selects run time configuration  */
            int riseFallTimes;       /* output rising and falling times? */
            int triggerWidth;        /* extra width for the trigger */
            int absorber;            /* lead, concrete, etc */
            int rawBoard;	     /* output raw board data? */
	    int barometerCalibrated; /* is barometer calibrated? */
            float barA, barB;        /* barometer calibration */
            char serialDevice[SERIAL_LENGTH];    /* serial device name*/
          }
        interfaceConfiguration;

#endif /* CONFIGURATION_H */














