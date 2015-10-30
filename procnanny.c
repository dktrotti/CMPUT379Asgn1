#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include "memwatch.h"

void waitandkill(pid_t targetpid, char* targetname, int seconds);
void logtext(char* info, bool debug);
void killcompetitors();
void clearlogfile();
void SIGINThandler(int sig);
void SIGHUPhandler(int sig);
bool inProcArray(int val);
void insertInProcArray(int val);
void removeFromProcArray(int val);

bool debug = true;
int interval = 5;
int killcount = 0;
int procarray[1024];

bool SIGHUPcaught = false;
bool SIGINTcaught = false;


//====================================================================================
//MAIN FUNCTION
//====================================================================================
int main (int argc, char* argv[]) {
	clearlogfile();
	char starttext[34];
	sprintf(starttext, "Info:  Parent process is PID %d", getpid());
	logtext(starttext, debug);

	signal(SIGINT, SIGINThandler);
	signal(SIGHUP, SIGHUPhandler);

  	int seconds = 0;
  	char currentprocess[255];
	FILE* configfp;

  	if (argc != 2) {
  		logtext("Error: Invalid number of arguments.", debug);
  		exit(1);
  	}

  	killcompetitors();
  	// Open config file.
  	configfp = fopen(argv[1], "r");

  	FILE* cmdfp;
  	int pid;

  	while (true) {
  		// TODO: Check flags to see if signals have been caught
  		if (SIGINTcaught) {
  			fclose(configfp);
  			killcompetitors();
  			exit(0);
  		}
	  	if (SIGHUPcaught) {
  			SIGHUPcaught = false;
  		}
	  	
	  	rewind(configfp);

  		// Read config entries
  		while (fscanf(configfp, "%s %d", currentprocess, &seconds) > 0) {
  			// Find processes with names exactly matching currentprocess
  			char command[260] = "pgrep -x ";
  	 		strcat(command, currentprocess);
  	 		cmdfp = popen(command, "r");

  	 		// Check if any processes exist
  	 		if (fscanf(cmdfp, "%u", &pid) <= 0) {
  	 	 		char buff[285];
  	 	 		sprintf(buff, "Info: No '%s' processes found", currentprocess);
  	 	 		logtext(buff, debug);
  	 		} else {
  	 			// If processes are found
  	 			if (!inProcArray(pid)) {
  	 				char inittext[312];
  	 				sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d)", currentprocess, pid);
  	 				logtext(inittext, debug);
  	 				insertInProcArray(pid);
  	 				waitandkill(pid, currentprocess, seconds);
  	 			}
  	 		}

  	 		// Continue reading processes from command
  	 		while(fscanf(cmdfp, "%u", &pid) > 0) {
  	 			if (!inProcArray(pid)) {
  	 				char inittext[312];
  	 				sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d)", currentprocess, pid);
  	 				logtext(inittext, debug);
  	 				insertInProcArray(pid);
  	 				waitandkill(pid, currentprocess, seconds);
  	 			}
  	 		}

  	 		pclose(cmdfp);
  		}
  		sleep(interval);
    }

  	return 0;
}
//====================================================================================


void waitandkill(pid_t targetpid, char* targetname, int seconds) {
  	pid_t pid;

  	fflush(stdout);
  	if ((pid = fork()) < 0) {
    	//err_sys("fork error");
    	logtext("Error: Fork failed.", debug);
	} else if (pid == 0) {
	    //Child process
	    sleep(seconds);
	    char infotext[56];
	    int rv = kill(targetpid, SIGKILL);
	    if (rv == 0) {
	      	//Success
	      	sprintf(infotext, "Action: PID %d (%s) killed after exceeding %d seconds", targetpid, targetname, seconds);
	      	logtext(infotext, debug);
	    } else {
	      	//Failed
	    }
	    exit(0);
	} else if (pid > 0) {
	    //Parent process
		return;
	}
}

void logtext(char* info, bool debug) {
  	time_t curtime;
  	struct tm * timeinfo;
  	char buff[80];
  	FILE* logpointer = fopen(getenv("PROCNANNYLOGS"), "a");

  	time(&curtime);
  	timeinfo = localtime(&curtime);
  	strftime(buff, sizeof(buff), "%a %b %d %T %Z %Y", timeinfo);
  	if (debug) {
  		printf("[%s] %s.\n", buff, info);
		fflush(stdout);
  	}
  	fprintf(logpointer, "[%s] %s.\n", buff, info);
  	fclose(logpointer);
}

void clearlogfile() {
  	FILE* logpointer = fopen(getenv("PROCNANNYLOGS"), "w");
  	fclose(logpointer);
}

void killcompetitors() {
	//THERE CAN ONLY BE ONE!!!
	FILE* compfp;
	pid_t ownpid = getpid();
	pid_t pid;
	compfp = popen("pgrep procnanny", "r");
	while(fscanf(compfp, "%u", &pid) > 0) {
		if (pid != ownpid) {
	    	kill(pid, SIGKILL);
		}
	}
}

void SIGINThandler(int sig) {
	char INTtext[128];
	sprintf(INTtext, "Info: Caught SIGINT. Exiting cleanly.  %d process(es) killed", killcount);
	logtext(INTtext, true);
	SIGINTcaught = true;
}

void SIGHUPhandler(int sig) {
	logtext("Info: Caught SIGHUP. Configuration file re-read", true);
	SIGHUPcaught = true;
}

bool inProcArray(int val) {
	int i;
	for (i = 0; i < 1024; i++) {
		if (procarray[i] == val) {
			return true;
		}
	}
	return false;
}

void insertInProcArray(int val) {
	int i;
	for (i = 0; i < 1024; i++) {
		if (procarray[i] == 0) {
			procarray[i] = val;
			break;
		}
	}
}

void removeFromProcArray(int val) {	
	int i;
	for (i = 0; i < 1024; i++) {
		if (procarray[i] == val) {
			procarray[i] = 0;
			break;
		}
	}
}