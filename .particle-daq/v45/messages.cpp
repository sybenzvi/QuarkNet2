#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include "quarknet.h"


void sendMessage(char *msg)
{
   ofstream serial("test");

   serial << msg;
   serial.close();
}


void getMessage(void)
{
   ifstream serial("test");
   char *msg;

   serial >> msg;
   cout << "\n" << msg << "\n";
   serial.close();
}
