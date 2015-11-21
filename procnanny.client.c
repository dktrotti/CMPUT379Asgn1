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

#define MAXMSG	512
#define INTERVAL 5

void SIGINThandler(int sig);
void killcompetitors();
bool inProcArray(int val);
void insertInProcArray(int val);
void removeFromProcArray(int val);
void writeToServer(int filedes, char* message);
int readFromServer(int filedes, char* buf);
void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port);

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
	char* serverhost = argv[1];
	int port = atoi(argv[2]);

	//Set up socket
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("socket (client)");
		exit (EXIT_FAILURE);
	}

	//Connect to the server
	init_sockaddr (&servername, serverhost, port);
	if (0 > connect (sock, (struct sockaddr *) &servername, sizeof (servername))) {
		perror ("connect (client)");
		exit (EXIT_FAILURE);
	}

	pipe(workpipe);
	pipe(killpipe);
	fcntl(killpipe[0], F_SETFL, O_NONBLOCK);

	killcompetitors();

	FILE* cmdfp;
	int pid;

	char config[MAXMSG];

	while (true) {
		if (SIGINTcaught) {
			killcompetitors();
			exit(0);
		}
		//Check for new config string
		writeToServer(sock, "cfg");
		readFromServer(sock, config);

		//Source: http://stackoverflow.com/a/17983619
		//Loop through config string
		char * curLine = config;
		while (curLine) {
			char * nextLine = strchr(curLine, '\n');
			if (nextLine) {
				*nextLine = '\0';  // temporarily terminate the current line
			}
			//Read and set the seconds value
			sscanf(curLine, "%s %d", currentprocess, &seconds);
			//Find PIDs for the process name
			//If none, tell server through socket
			//Loop through PIDs
				//Check for PID in monitored PIDs
				//Tell server about monitoring
				//Spawn child if needed and begin monitoring
			if (nextLine) {
				*nextLine = '\n';  // then restore newline-char, just to be tidy
			}
			curLine = nextLine ? (nextLine+1) : NULL;
		}
		//Wait
		sleep(INTERVAL);
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

void writeToServer(int filedes, char* message) {
	int nbytes;

	nbytes = write(filedes, message, strlen(message) + 1);
	if (nbytes < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

int readFromServer(int filedes, char* buf) {
	int nbytes;

	nbytes = read(filedes, buf, MAXMSG);
	if (nbytes < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	} else if (nbytes == 0) {
		//EOF
		return -1;
	} else {
		return 0;
	}
}

//Source: http://www.gnu.org/software/libc/manual/html_node/Inet-Example.html
void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port) {
	struct hostent *hostinfo;

	name->sin_family = AF_INET;
	name->sin_port = htons (port);
	hostinfo = gethostbyname (hostname);
	if (hostinfo == NULL) {
		fprintf (stderr, "Unknown host %s.\n", hostname);
		exit (EXIT_FAILURE);
	}
	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}