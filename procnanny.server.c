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
#include <arpa/inet.h>
#include "memwatch.h"

#define PORT 7701
#define MAXMSG	512
#define INTERVAL 5

void clearlogfile();
void killcompetitors();
void logtext(char* info, bool debug);
void SIGHUPhandler(int sig);
void SIGINThandler(int sig);
void writeToClient(int filedes, char* message);
int readFromClient(int filedes, char* buffer);
int makeSocket(uint16_t port);
void readConfigFile(char* location, char* buffer);

bool debug = true;
int killcount = 0;

char ownhostname[256];
char nodelist[4096];
char config[MAXMSG];
char configpath[256];

bool SIGHUPcaught = false;
bool SIGINTcaught = false;
//====================================================================================
//MAIN FUNCTION
//====================================================================================
int main (int argc, char* argv[]) {
	//argv[1] == config file location
	sprintf(configpath, "%s", argv[1]);

	signal(SIGHUP, SIGHUPhandler);
	signal(SIGINT, SIGINThandler);

	clearlogfile();
	
	if (argc != 2) {
		logtext("Error: Invalid number of arguments.", debug);
		exit(1);
	}

	if (gethostname(ownhostname, 256) < 0) {
		logtext("Error: Failed to get hostname.", debug);
	}

	//Log PID/compname
	char introstr[512];
	sprintf(introstr, "procnanny server: PID %d on node %s, port %d", getpid(), ownhostname, PORT);
	logtext(introstr, true);

	FILE* infopointer = fopen(getenv("PROCNANNYSERVERINFO"), "w");
	fprintf(infopointer, "NODE %s PID %d PORT %d", ownhostname, getpid(), PORT);
	fclose(infopointer);

	//Set up socket
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	size_t size;
	char databuf[MAXMSG];
	struct timeval tv;

	sock = makeSocket(PORT);
	if (listen (sock, 1) < 0) {
		perror ("listen");
		exit (EXIT_FAILURE);
	}

	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	//Read config file
	memset(config, '\0', sizeof(config));
	readConfigFile(argv[1], config);

	while (true) {
		read_fd_set = active_fd_set;
		tv.tv_sec = INTERVAL;
		tv.tv_usec = 0;
		
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, &tv) < 0) {
			// perror ("select");
			// exit (EXIT_FAILURE);
		}

		if (SIGHUPcaught) {
			//Read config file
			memset(config, '\0', sizeof(config));
			readConfigFile(argv[1], config);
			logtext("Info: Caught SIGHUP. Configuration file re-read", true);
			SIGHUPcaught = false;
			continue;
		}

		if (SIGINTcaught) {
			//Do something
			char INTtext[128];
			sprintf(INTtext, "Info: Caught SIGINT. Exiting cleanly.  %d process(es) killed on%s", killcount, nodelist);
			logtext(INTtext, true);
			exit(0);
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

					//Get client name
					//Source: http://stackoverflow.com/a/10236702
					struct in_addr ipv4addr;

					inet_pton(AF_INET, inet_ntoa(clientname.sin_addr), &ipv4addr);
					sprintf(nodelist, "%s %s", nodelist, gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET)->h_name);
				} else {
					//Data arriving on an already-connected socket
					if (readFromClient (i, databuf) < 0) {
						close (i);
						FD_CLR (i, &active_fd_set);
					} else {
						//Do something with data
						if (strcmp(databuf, "cfg") == 0) {
							//Send config file
							writeToClient(i, config);
						} else {
							//Read responses from clients
							logtext(databuf, debug);
							if (strncmp(databuf, "Action: PID", 11) == 0) {
								killcount++;
							}
						}
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

void killcompetitors() {
	//THERE CAN ONLY BE ONE!!!
	FILE* compfp;
	pid_t ownpid = getpid();
	pid_t pid;
	compfp = popen("pgrep procnanny.server", "r");
	while(fscanf(compfp, "%u", &pid) > 0) {
		if (pid != ownpid) {
			kill(pid, SIGKILL);
		}
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

void SIGHUPhandler(int sig) {
	SIGHUPcaught = true;

	// logtext("Info: Caught SIGHUP. Configuration file re-read", true);
	// memset(config, '\0', sizeof(config));
	// readConfigFile(configpath, config);
}

void SIGINThandler(int sig) {
	SIGINTcaught = true;

	// char INTtext[128];
	// sprintf(INTtext, "Info: Caught SIGINT. Exiting cleanly.  %d process(es) killed on %s", killcount, nodelist);
	// logtext(INTtext, true);
	// exit(0);
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

void writeToClient(int filedes, char* message) {
	int nbytes;

	nbytes = write(filedes, message, strlen(message) + 1);
	if (nbytes < 0) {
		perror("write");
		exit(EXIT_FAILURE);
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