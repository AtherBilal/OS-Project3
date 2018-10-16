#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <signal.h>
#include "validation.h"
#include <unistd.h>



//Shared Memory
int shmClockId;
int shmshmMsgId;
void* shmClockAddress;
void* shmMsgAddress;
int semID;

FILE* randomNumberFP;
void checkForErrors(char programName[], int errnoValue);
void calculateEndingTime(int startTime[], int endTime[], int duration);
void deallocateMemory();
void closeFilePointers();
void exitCleanUp();
long convertToNanoSeconds(int clockArray[]);
void handleCtrlC();
void timeOutExit();


int main(int argc, char *argv[]){
  //Signal Handling
  signal(SIGINT, handleCtrlC);
  signal(SIGTERM, timeOutExit);

  randomNumberFP = fopen("/dev/urandom", "r");
  checkForErrors(argv[0], errno);

  //Generate keys
  key_t shmtime_key = ftok("./CreateKeyFile", 1);
  key_t shmshmMsg_key = ftok("./CreateKeyFile", 2);
  key_t sem_key = ftok("./CreateKeyFile", 3);
  checkForErrors(argv[0], errno);

  //Attach to shared memory
  shmClockId = shmget(shmtime_key, sizeof(int) * 2, 0);
  shmshmMsgId = shmget(shmshmMsg_key, sizeof(int) * 3, 0);
  shmClockAddress = shmat(shmClockId, (void*)0, 0);
  shmMsgAddress = shmat(shmshmMsgId, (void*)0, 0);
  checkForErrors(argv[0], errno);
  //Semaphore Setup
  semID = semget(sem_key, 1, 0);
  struct sembuf sop;
  checkForErrors(argv[0], errno);

  int currTimeSnapshot[2];
  int* currTime = shmClockAddress;
  int endTime[2];
  memcpy(currTimeSnapshot, shmClockAddress, 8);

  // get termination time
  unsigned int seed;
  fread(&seed, sizeof(unsigned int), 1, randomNumberFP);
  calculateEndingTime(currTimeSnapshot, endTime, (rand_r(&seed) % 1000000) + 1);
  printf("end time: 0:%d, 1:%d\n",endTime[0],endTime[1] );

  long endTimeNanoSeconds = convertToNanoSeconds(endTime);
  long currTimeNanoseconds;

  int timeCheck = 0;
  while (!timeCheck) {
    currTimeNanoseconds = convertToNanoSeconds(currTime);
    if (endTimeNanoSeconds <= currTimeNanoseconds) {
      timeCheck = 1;
    }
  };

  int hasWrittenToShmshmMsg = 0;
  int* shmMsg = shmMsgAddress;

  while (!hasWrittenToShmshmMsg) {
    //critical section
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    semop(semID, &sop, 1);

    if (shmMsg[0] == -1 && shmMsg[1] == -1 && shmMsg[2] == 0) {
      shmMsg[0] = endTime[0];
      shmMsg[1] = endTime[1];
      shmMsg[2] = getpid();
      // break loop
      hasWrittenToShmshmMsg = 1;
    }

    sop.sem_op = 1; 
    semop(semID, &sop, 1);
  }
  exitCleanUp();
}

void exitCleanUp() {
  deallocateMemory();
  closeFilePointers();
  exit(0);
}
void calculateEndingTime(int startTime[], int endTime[], int duration) {
  int seconds_end = startTime[0];
  int nanoseconds_end = startTime[1] + duration;

  if (nanoseconds_end > 1000000000) {
    nanoseconds_end = nanoseconds_end % 1000000000;
    seconds_end += 1;
  }

  endTime[0] = seconds_end;
  endTime[1] = nanoseconds_end;

}

// only works with clockArray where clockArray[0] is seconds and clockArray[1] is nanoseconds
long convertToNanoSeconds(int clockArray[]){
  return (long)(clockArray[0] * 1000000000 + clockArray[1]);
}

void checkForErrors(char programName[], int errnoValue){
  if (errnoValue) {
    fprintf(stderr, "%s: Error: %s\n", programName, strerror(errno));
    deallocateMemory();
    exit (1);
  }
}

void deallocateMemory() {
if(shmClockAddress)
  shmdt(shmClockAddress);
if(shmMsgAddress)
  shmdt(shmMsgAddress);
}


void closeFilePointers() {
  if (randomNumberFP) {
    fclose(randomNumberFP);
  }
}


void handleCtrlC() {
  fprintf(stderr, "PID: %ld interrupted by signal. Exitting!\n", (long)getpid());
  closeFilePointers();
  deallocateMemory();
  exit(1);
}

void timeOutExit() {
  fprintf(stderr, "PID: %ld timing out because of signal from master\n", (long)getpid());
  closeFilePointers();
  deallocateMemory();
  exit(1);
}
