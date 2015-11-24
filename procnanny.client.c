#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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
void spawnchild();

bool SIGINTcaught = false;
int procarray[1024];

int workpipe[2];
int killpipe[2];

int sock;

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
	struct sockaddr_in servername;

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
	int numbytes;
	char readbuf[2];

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
			char temp[MAXMSG];
			writeToServer(sock, temp);





			char command[260] = "pgrep -x ";
  	 		strcat(command, currentprocess);
  	 		cmdfp = popen(command, "r");

  	 		// Check if any processes exist
  	 		if (fscanf(cmdfp, "%u", &pid) <= 0) {
  	 	 		char buff[285];


  	 	 		sprintf(buff, "Info: No '%s' processes found on node whatever", currentprocess);
  	 	 		writeToServer(sock, buff);


  	 		} else {
  	 			// If processes are found
  	 			if (!inProcArray(pid)) {
  	 				char inittext[312];


  	 				sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d) on node whatever", currentprocess, pid);
  	 				writeToServer(sock, inittext);


  	 				insertInProcArray(pid);

					numbytes = read(killpipe[0], readbuf, sizeof(readbuf));
  	 				if (numbytes == -1) {
						//No children available
						spawnchild(workpipe, killpipe);
					} else {
						//Children available
					}

					char workstring[260];
					sprintf(workstring, "%s %d %d\n", currentprocess, pid, seconds);
					write(workpipe[1], workstring, sizeof(workstring));
  	 			}
  	 		}

  	 		// Continue reading processes from command
  	 		while(fscanf(cmdfp, "%u", &pid) > 0) {
  	 			if (!inProcArray(pid)) {
  	 				char inittext[312];


  	 				sprintf(inittext, "Info: Initializing monitoring of process '%s' (PID %d) on node whatever", currentprocess, pid);
  	 				writeToServer(sock, inittext);


  	 				insertInProcArray(pid);

					numbytes = read(killpipe[0], readbuf, sizeof(readbuf));
  	 				if (numbytes == -1) {
						//No children available
						spawnchild(workpipe, killpipe);
					} else {
						//Children available
					}

					char workstring[260];
					sprintf(workstring, "%s %d %d\n", currentprocess, pid, seconds);
					write(workpipe[1], workstring, sizeof(workstring));
  	 			}
  	 		}

  	 		pclose(cmdfp);





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
	compfp = popen("pgrep procnanny.client", "r");
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

void spawnchild() {
	pid_t pid;

	if((pid = fork()) < 0) {
		//Fork error
	} else if (pid == 0) {
		//Child process
	    close(workpipe[1]); //Close writing end of workpipe
	    close(killpipe[0]); //Close reading end of killpipe
	    char targetname[256];
	    pid_t targetpid;
	    int seconds;
	    while(true) {
	    	char buf[260];
	    	int readval = read(workpipe[0], buf, sizeof(buf));
	    	if (readval == 0) {
	    		exit(0);
	    	}
	    	sscanf(buf, "%s %d %d", targetname, &targetpid, &seconds);
	    	sleep(seconds);
	    	int killval = kill(targetpid, SIGKILL);
	    	if (killval == 0) {
	      		//Success
	      		char infotext[320];


	      		sprintf(infotext, "Action: PID %d (%s) killed after exceeding %d seconds on node whatever", targetpid, targetname, seconds);
	      		writeToServer(sock, infotext);


	    		write(killpipe[1], "k\n", 2);
	    	} else {
	    		write(killpipe[1], "n\n", 2);
	    	}
	    }
	} else if (pid > 0) {
		//Parent process
		return;
	}
}