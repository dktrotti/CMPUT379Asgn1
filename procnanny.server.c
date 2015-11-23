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
#include <sys/time.h>
#include "memwatch.h"

#define PORT 7701
#define MAXMSG	512
#define INTERVAL 5

void clearlogfile();
void logtext(char* info, bool debug);
void SIGHUPhandler(int sig);
void SIGINThandler(int sig);
int readFromClient(int filedes, char* buffer);
int makeSocket(uint16_t port);
void readConfigFile(char* location, char* buffer);

bool debug = true;
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

	//Set up socket
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	size_t size;
	char databuf[MAXMSG];
	struct timeval tv;
	char config[MAXMSG];

	sock = make_socket (port);
	if (listen (sock, 1) < 0) {
		perror ("listen");
		exit (EXIT_FAILURE);
	}

	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	//Open config file
	readConfigFile(argv[1], config);
	printf("%s\n", config);

	while (true) {
		if (SIGHUPhandler) {
			//Do something
		}

		if (SIGINTcaught) {
			//Do something
		}
		read_fd_set = active_fd_set;
		tv.tv_sec = INTERVAL;
		tv.tv_usec = 0;
		
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, tv) < 0) {
			perror ("select");
			exit (EXIT_FAILURE);
		}

		//Service all the sockets with input pending
		for (i = 0; i < FD_SETSIZE; ++i) {
			memset(databuf, '\0', sizeof(databuf));
			if (FD_ISSET (i, &read_fd_set)) {
				if (i == sock) {
					//Connection request on original socket
					int new;
					size = sizeof (clientname);
					new = accept (sock, (struct sockaddr *) &clientname, &size);
					if (new < 0) {
						perror ("accept");
						exit (EXIT_FAILURE);
					}
					FD_SET (new, &active_fd_set);
				} else {
					//Data arriving on an already-connected socket
					if (readFromClient (i, databuf) < 0) {
						close (i);
						FD_CLR (i, &active_fd_set);
					} else {
						//Do something with data
						//Send config file
						//Read responses from clients
					}
				}
			}
		}
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

int readFromClient(int filedes, char* buffer) {
	int nbytes;

	nbytes = read(filedes, buffer, MAXMSG);
	if (nbytes < 0) {
		/* Read error. */
		perror ("read");
		exit (EXIT_FAILURE);
	} else if (nbytes == 0) {
		/* End-of-file. */
		return -1;
	} else {
		return 0;
	}
}

int makeSocket(uint16_t port) {
	int sock;
	struct sockaddr_in name;

	//Create the socket
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	//Give the socket a name
	name.sin_family = AF_INET;
	name.sin_port = htons (port);
	name.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	return sock;
}

void readConfigFile(char* location, char* buffer) {
	FILE* fp = fopen(location, "r");
	fread(buffer, sizeof(char), MAXMSG, fp);
	fclose(fp);
}