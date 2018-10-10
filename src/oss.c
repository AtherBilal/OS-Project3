#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include "validation.h"


// log files
char* log_filename;
FILE* log_fp = NULL;

// shared memory
int shmClockId;
int shmMsgId;
void* shmClockAddress;
void* shmMsgAddress;
void deallocateMemory();
void checkForErrors(char programName[], int errnoValue);

int main(int argc, char *argv[]){
  int maxRunTime = 2;

  static char usageMessage[] = "%s: Usage: %s [-s maximum number of user processes spawned (Default 5)] [-l log file name] [-t max run time][-h help]\n";
  static char helpMessage[] = "-s  Maximum number of user processes spawned Default 5.\n\
-l    Path to log file. File must be writable.\n\
-t    The time in seconds when the master will terminate itself and all children.\n";

// set signals TODO:
// alarm(2);
// signal(SIGALRM, timeoutKillAll);
// signal(SIGINT, handleCtrlC);
int customErrorCode, option;
int maxProcesses = -1;
extern char *optarg;
//TODO: Add colons

while ((option = getopt(argc, argv, "hslt")) != -1) {
  switch (option) {
    case('h'):
     printf("%s%s\n", argv[0], helpMessage);
     return 0;
   case('s'):
     if (isNumber(optarg)) {
       maxProcesses = (int)strtol(optarg, NULL, 0);
     } else {
       customErrorCode = 1;
     }
     break;
   case('l'):
     log_fp = fopen(optarg, "w");
     log_filename = optarg;
     break;
   case('t'):
     if (isNumber(optarg)) {
       maxRunTime = (int)strtol(optarg, NULL, 0);
     } else {
       customErrorCode = 2;
     }
     break;
   case ('?'):
     customErrorCode = 3;
     break;
  }
}
printf("It works?!\n" );
// set to default
if (maxProcesses == -1) {
  maxProcesses = 5;
}
// alarm(maxRunTime);
key_t shmClockKey = ftok("./CreateKeyFile", 1);
key_t shmMsgKey = ftok("./CreateKeyFile", 2);
key_t semKey = ftok("./CreateKeyFile", 3);
checkForErrors(argv[0], errno);



shmClockId = shmget(shmClockKey, sizeof(int) * 2, IPC_CREAT | 0644);
shmMsgId= shmget(shmMsgKey, sizeof(int) * 3, IPC_CREAT | 0644);
checkForErrors(argv[0], errno);

shmMsgAddress = shmat(shmClockId, (void*)0, 0);
shmClockAddress = shmat(shmMsgId, (void*)0, 0);
checkForErrors(argv[0], errno);

// setup clock
int* shmClock = shmClockAddress;
int* shmMsg   = shmMsgAddress;

shmClock[0] = 0, shmClock[1] = 0; //subscript 0 -> seconds, subscript 1 -> nanoseconds
shmMsg[0] = -1, shmMsg[1] = -1, shmMsg[2] = -2; //subscript 0 -> seconds, subscript 1 -> nanoseconds, subscript 2 -> PID

deallocateMemory();
}

void checkForErrors(char programName[], int errnoValue){
  if (errnoValue) {
    fprintf(stderr, "%s: Error: %s\n", programName, strerror(errno));
    deallocateMemory();
    exit (1);
  }
}

void deallocateMemory() {
  if (shmClockId)
    shmctl(shmClockId, IPC_RMID, NULL);
  if (shmMsgId)
    shmctl(shmMsgId, IPC_RMID, NULL);
}
