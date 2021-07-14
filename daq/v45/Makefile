CFLAGS = -g -D__USE_GNU
# fake custom runs by filtering
#CFLAGS += -DFAKECUSTOM
# fix time walk in sum of timestamps if detected relative to wall clock
#  (not essential for functionality and generates loads of output)
#CFLAGS += -DFIXTIMEWALK
# debug output for serial_sender, key_sender and time_sender
#  remember... this may not be the desired debugging; you can also
#  set a debug mode in the program itself...
#CFLAGS += -DDEBUG
# crude fake serial input for debugging without board
#CFLAGS += -DFAKESERIAL
#echo muon
#CFLAGS += -DECHOMUON

KERNEL_VERSION = $(shell uname -r | cut -d\. --fields=1,2)
ifeq (2.4,$(KERNEL_VERSION))
  CFLAGS += -DREDHAT7
endif
ifeq (2.6,$(KERNEL_VERSION))
  CFLAGS += -DREDHAT7
endif
ifeq (4.4,$(KERNEL_VERSION))
  CFLAGS += -DREDHAT7
endif
#CFLAGS += -DREDHAT7

#CC = g++ $(CFLAGS)
CC = gcc $(CFLAGS)

DEPS = Makefile messages.cpp quarknet.h queue_defs.h configuration.h

all: run.e serial_sender.e key_sender.e time_sender.e

run.e: interface.c menu.c $(DEPS)
	$(CC) $(filter %.c,$^) -o run.e -lm

serial_sender.e: serial_sender.c $(DEPS)
	$(CC) $(filter %.c,$^) -o serial_sender.e -lm

key_sender.e: key_sender.c $(DEPS)
	$(CC) $(filter %.c,$^) -o key_sender.e -lm

time_sender.e: time_sender.c $(DEPS)
	$(CC) $(filter %.c,$^) -o time_sender.e -lm

clean:
	rm -f run.e serial_sender.e key_sender.e time_sender.e

veryclean: clean
	rm -f *~ */*~ */*/*~

