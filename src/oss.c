#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include "validation.h"


// log files
char* log_filename;
FILE* logFilePointer = NULL;

// shared memory
int shmClockId;
int shmMsgId;
void* shmClockAddress;
void* shmMsgAddress;
int maxRunTime = -1;
//Semaphore
int semID;

void handleCtrlC();
void closeFilePointers();
void deallocateMemory();
void timeoutKillAll();
void cleanUpExit();
void checkForErrors(char programName[], int errnoValue);
void checkTotalMaxProcessesLimitReached(int totalProcessCount, char programName[]);
void printUsageMsg(char programName[], int errorOut);


int main(int argc, char *argv[]){

signal(SIGALRM, timeoutKillAll);
signal(SIGINT, handleCtrlC);
int option;
int maxProcesses = -1;
extern char *optarg;
extern int optind;

while ((option = getopt(argc, argv, "hs:l:t:")) != -1) {
  switch (option) {
    case('h'):
     printUsageMsg(argv[0], 0);
   case('s'):
     if (isNumber(optarg)) {
       maxProcesses = (int)strtol(optarg, NULL, 0);
     } else {
       printUsageMsg(argv[0], 1);
     }
     break;
   case('l'):
     logFilePointer = fopen(optarg, "w");
     log_filename = optarg;
     break;
   case('t'):
     if (isNumber(optarg)) {
       maxRunTime = (int)strtol(optarg, NULL, 0);
     } else {
       printUsageMsg(argv[0], 1);
     }
     break;
   case ('?'):
      printUsageMsg(argv[0], 1);
     break;
  }
}
// set to default
if (maxProcesses == -1) {
  maxProcesses = 5;
}
if (maxRunTime == -1){
  maxRunTime = 2;
}
alarm(maxRunTime);
printf("maxProcesses: %d\n", maxProcesses);
// create keys
key_t shmClockKey = ftok("./CreateKeyFile", 1);
key_t shmMsgKey = ftok("./CreateKeyFile", 2);
key_t semKey = ftok("./CreateKeyFile", 3);
checkForErrors(argv[0], errno);

// create shared memory
// 0644 makes it read only http://permissions-calculator.org/decode/0644/
shmClockId = shmget(shmClockKey, sizeof(int) * 2, IPC_CREAT | 0644);
shmMsgId = shmget(shmMsgKey, sizeof(int) * 3, IPC_CREAT | 0644);
checkForErrors(argv[0], errno);

// attach to shared memory
shmMsgAddress = shmat(shmMsgId, (void*)0, 0);
shmClockAddress = shmat(shmClockId, (void*)0, 0);
checkForErrors(argv[0], errno);

// semaphore setup
printf("creating semaphore\n");
semID = semget(semKey, 1, IPC_CREAT | 0666);
checkForErrors(argv[0], errno);
union semun semArg;
semArg.val = 1;
semctl(semID, 0, SETVAL, semArg);
checkForErrors(argv[0], errno);


// setup clock
int* shmClock = shmClockAddress;
int* shmMsg   = shmMsgAddress;

shmClock[0] = 0, shmClock[1] = 0; //subscript 0 -> seconds, subscript 1 -> nanoseconds
shmMsg[0] = -1, shmMsg[1] = -1, shmMsg[2] = 0; //subscript 0 -> seconds, subscript 1 -> nanoseconds, subscript 2 -> PID

pid_t pid;
int runningProcessCount = 0;
int totalProcessCount = 0;
for (int i = 0; i < maxProcesses; i++) {
  pid = fork();
  // checkForErrors(argv[0], errno);
  if (pid == 0) {
    execl("./child", "child", NULL);
  }
  if (pid) {
    runningProcessCount++;
    totalProcessCount++;
    checkTotalMaxProcessesLimitReached(totalProcessCount, argv[0]);
  }
}

        //break when 2 seconds have passed
  for (; shmClock[0] < 2; shmClock[1] += 100) {
    // printf("total processes: %d\n", totalProcessCount);

    //Increment seconds if enough nanoseconds
    // printf("incrementing nanoseconds\n");
    if (shmClock[1] > 1000000000) {
      shmClock[0]++;
      shmClock[1] = shmClock[1] % 1000000000;
      fprintf(stderr, "INCREMENTED SECONDS\n");
    }
    //Spawn processes up until max processes are in use
    while (runningProcessCount < maxProcesses) {
      pid = fork();
      // checkForErrors(argv[0], errno);
      if (pid == 0) {
        execl("./child", "child", NULL);
      }
      if (pid) {
        runningProcessCount++;
        totalProcessCount++;
        checkTotalMaxProcessesLimitReached(totalProcessCount, argv[0]);
      }
    }

    //Check if a slave has written to shared message
    if (shmMsg[0] != -1 && shmMsg[1] != -1 && shmMsg[2] != 0) {
      //Write to file.
      // shmMsg[0] -> seconds
      // shmMsg[1] -> nanoseconds
      // shmMsg[2] -> child pid
      fprintf(logFilePointer, "%s: Child %ld is terminating at my time %d.%d because it reached %d.%d in user.\n", argv[0], (long)shmMsg[2], shmClock[0], shmClock[1], shmMsg[0], shmMsg[1]);
      // printf("finished printing to file\n");
      shmMsg[0] = -1, shmMsg[1] = -1, shmMsg[2] = 0;

      waitpid(shmMsg[2], NULL, 0);
      checkForErrors(argv[0], errno);
      runningProcessCount--;
    }
  }
// wait(NULL);
fprintf(stderr,"Master process exiting due to timeout. Killing children processes\n");
cleanUpExit();
}

void cleanUpExit(){
  deallocateMemory();
  closeFilePointers();
}

void checkTotalMaxProcessesLimitReached(int totalProcessCount, char programName[]){
  if (totalProcessCount > 100){
    printf("%s: terminating due to limit of 100 processes reached", programName);
    cleanUpExit();
  }
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
  if (shmMsgAddress){
    shmdt(shmMsgAddress);
  }
  if (shmClockAddress){
    shmdt(shmClockAddress);
  }
  semctl(semID, 0, IPC_RMID);
}

void handleCtrlC() {
  deallocateMemory();
  fprintf(stderr,"Master process exiting due to exit signal. Killing children processes\n");
  kill(0, SIGINT);
  exit(1);
}

void timeoutKillAll()
{
  fprintf(stderr,"Master process exiting due to timeout. Killing children processes\n");
  deallocateMemory();
  closeFilePointers();
  kill(0, SIGTERM);
  exit(1);

}


void closeFilePointers() {
  if (logFilePointer) {
    printf("closing logFilePointer\n");
    fclose(logFilePointer);
  }
}

void printUsageMsg(char programName[], int errorOut){
  printf("%s: Usage: %s [-s maximum number of user processes spawned (Default 5)] [-l log file name] [-t max run time (Default 2 seconds)]\n", programName, programName);
  exit(errorOut);
}