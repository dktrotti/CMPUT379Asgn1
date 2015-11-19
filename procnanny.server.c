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

void clearlogfile();
void logtext(char* info, bool debug);
void SIGHUPhandler(int sig);
void SIGINThandler(int sig);

bool debug = false;
int killcount = 0;

//====================================================================================
//MAIN FUNCTION
//====================================================================================
int main (int argc, char* argv[]) {
	//argv[1] == config file
	signal(SIGHUP, SIGHUPhandler);
	signal(SIGINT, SIGINThandler);

	clearlogfile();
	
	if (argc != 2) {
		logtext("Error: Invalid number of arguments.", debug);
		exit(1);
	}

	//Log PID/compname

	//Open config file
	while (true) {
		if (SIGHUPhandler) {
			//Do something
		}

		if (SIGINTcaught) {
			//Do something
		}

		//Send config file
		//Read responses from clients
		//Wait
	}

}
//====================================================================================

void clearlogfile() {
	FILE* logpointer = fopen(getenv("PROCNANNYLOGS"), "w");
	fclose(logpointer);
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

void SIGHUPhandler(int sig) {
	SIGHUPcaught = true;
}

void SIGINThandler(int sig) {
	SIGINTcaught = true;
}