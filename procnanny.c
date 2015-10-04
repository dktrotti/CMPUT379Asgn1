#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "memwatch.h"

void waitandkill(pid_t targetpid, char* targetname, int seconds);
void logtext(char* info);
void killcompetitors();
void clearlogfile();

//====================================================================================
//MAIN FUNCTION
//====================================================================================
int main (int argc, char* argv[]) {
	clearlogfile();

	FILE* fp;
  	int seconds = 0;
  	char currentprocess[255];
  	int killcount = 0;

  	if (argc != 2) {
  		logtext("Error: Invalid number of arguments.");
  		exit(1);
  	}

  	killcompetitors();
  	fp = fopen(argv[1], "r");

  	fscanf(fp, "%u", &seconds);

  	FILE* cmdfp;
  	int pid;
  	while (fscanf(fp, "%s", currentprocess) > 0) {
  		char command[260] = "pgrep ";
  	 	strcat(command, currentprocess);
  	 	cmdfp = popen(command, "r");

  	 	if (fscanf(cmdfp, "%u", &pid) <= 0) {
  	 	 	char buff[285];
  	 	 	sprintf(buff, "Info: No '%s' processes found", currentprocess);
  	 	 	logtext(buff);
  	 	} else {
  	 		char inittext[312];
  	 		sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d)", currentprocess, pid);
  	 		logtext(inittext);
  	 		waitandkill(pid, currentprocess, seconds);
  	 		killcount++;
  	 	}

  	 	while(fscanf(cmdfp, "%u", &pid) > 0) {
  	 		char inittext[312];
  	 		sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d)", currentprocess, pid);
  	 		logtext(inittext);
  	 		waitandkill(pid, currentprocess, seconds);
  	 		killcount++;
  	 	}

  	 	pclose(cmdfp);
  	}

  	sleep(seconds + 2);
  	char exittext[40];
  	sprintf(exittext, "Info: Exiting. %d process(es) killed", killcount);
  	logtext(exittext);
  	return 0;
}
//====================================================================================


void waitandkill(pid_t targetpid, char* targetname, int seconds) {
  	pid_t pid;

  	fflush(stdout);
  	if ((pid = fork()) < 0) {
    	//err_sys("fork error");
    	logtext("Error: Fork failed.");
	} else if (pid == 0) {
	    //Child process
	    sleep(seconds);
	    char infotext[56];
	    int rv = kill(targetpid, SIGKILL);
	    if (rv == 0) {
	      	//Success
	      	sprintf(infotext, "Action: PID %d (%s) killed after exceeding %d seconds", targetpid, targetname, seconds);
	      	logtext(infotext);
	    } else {
	      	//Failed
	    }
	    exit(0);
	} else if (pid > 0) {
	    //Parent process
		return;
	}
}

void logtext(char* info) {
  	time_t curtime;
  	struct tm * timeinfo;
  	char buff[80];
  	FILE* logpointer = fopen(getenv("PROCNANNYLOGS"), "a");

  	time(&curtime);
  	timeinfo = localtime(&curtime);
  	strftime(buff, sizeof(buff), "%a %b %d %T %Z %Y", timeinfo);
  	//printf("[%s] %s.\n", buff, info);
	//fflush(stdout);
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