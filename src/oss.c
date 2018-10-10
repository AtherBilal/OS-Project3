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

int main(int argc, char *argv[]){
  int maxRunTime;

  static char usageMessage[] = "%s: Usage: %s [-s maximum number of user processes spawned (Default 5)] [-l log file name] [-t max run time][-h help]\n";
  static char helpMessage[] = "-s  Maximum number of user processes spawned Default 5.\n\
-l    Path to log file. File must be writable.\n\
-t    The time in seconds when the master will terminate itself and all children.\n";

// set signals TODO:
// alarm(2);
// signal(SIGALRM, timeoutKillAll);
// signal(SIGINT, handleCtrlC);
int customErrorCode, n, option;
extern char *optarg;
//TODO: Add colons
while ((option = getopt(argc, argv, "hslt")) != -1) {
  switch (option) {
    case('h'):
     printf("%s%s\n", argv[0], helpMessage);
     return 0;
   case('n'):
     if (isNumber(optarg)) {
       n = (int)strtol(optarg, NULL, 0);
     } else {
       customErrorCode = 1;
     }
     break;
   case('s'):
     printf("option s needs to be implemented\n");
     break;
   case ('?'):
     customErrorCode = 3;
     break;
  }
}

}
