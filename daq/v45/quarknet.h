// QUARKNET.H - Chief header file for muon telescope project
//
//              VERSION 1.0 (7.12.00)
//


#ifndef __QUARKNET_H
#define __QUARKNET_H

#include "configuration.h"

// PROTOTYPES ///////////////////////////////////////////////
char mainMenu(char*,interfaceConfiguration*);
unsigned char CRWriteMenu(unsigned char);
void CRContents(unsigned char);
void InterfaceConfigMenu(interfaceConfiguration*, FILE*);
char* receiveKeyString(char*);
char receiveKeyChar(void); 
void serialSend(FILE*, const char*);

void sendMessage(char *);
void getMessage(void);

// new board only functions
void getCounts(int *, int *, int *, int *, int *, FILE *, int);
unsigned char getCRStatus(FILE *, int);
float readBarometer(FILE *, int, int, int, FILE *);
float readThermometer(FILE *, int);
int readGPS(FILE *serial, int qid, FILE*);
int getGPSfrq(FILE *serial, int qid);

#endif /* __QUARKNET_H */


