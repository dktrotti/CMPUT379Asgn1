#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include "memwatch.h"

void SIGINThandler(int sig);
void killcompetitors();
bool inProcArray(int val);
void insertInProcArray(int val);
void removeFromProcArray(int val);

bool SIGINTcaught = false;
int procarray[1024];

int workpipe[2];
int killpipe[2];

//====================================================================================
//MAIN FUNCTION
//====================================================================================
int main (int argc, char* argv[]) {
	//argv[1] == location
	//argv[2] == port
	signal(SIGINT, SIGINThandler);

	int seconds = 0;
	char currentprocess[255];

	if (argc != 3) {
		//logtext("Error: Invalid number of arguments.", debug);
		exit(1);
	}

	//Set up socket

	pipe(workpipe);
	pipe(killpipe);
	fcntl(killpipe[0], F_SETFL, O_NONBLOCK);

	killcompetitors();

	FILE* cmdfp;
	int pid;

	while (true) {
		if (SIGINTcaught) {
			killcompetitors();
			exit(0);
		}
		//Check for new config string

		//Loop through config string
			//Read and set the seconds value
			//Find PIDs for the process name
			//If none, tell server through socket
			//Loop through PIDs
				//Check for PID in monitored PIDs
				//Tell server about monitoring
				//Spawn child if needed and begin monitoring
		//Wait
	}
}
//====================================================================================

void SIGINThandler(int sig) {
	SIGINTcaught = true;
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