//Source: http://www.gnu.org/software/libc/manual/html_node/Server-Example.html

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#define MAXMSG	512

int
read_from_client (int filedes)
{
	char buffer[MAXMSG];
	int nbytes;

	nbytes = read (filedes, buffer, MAXMSG);
	if (nbytes < 0) {
		/* Read error. */
		perror ("read");
		exit (EXIT_FAILURE);
	} else if (nbytes == 0) {
		/* End-of-file. */
		return -1;
	} else {
		/* Data read. */
		if (strcmp(buffer, "cfg") == 0) {
			printf("Sending config\n");
			write(filedes, "pname1 120\npname2 60\npname3 40\n", 512);
		}
		printf ("Server: got message: '%s'\n", buffer);
		return 0;
	}
}

int make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }

  return sock;
}

int main (int argc, char* argv[]) {
    if (argc != 2) {
        printf("Invalid number of arguments\n");
        exit(1);
    }
    int port = atoi(argv[1]);

	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	size_t size;

	/* Create the socket and set it up to accept connections. */
	printf("Making socket\n");
	sock = make_socket (port);
	if (listen (sock, 1) < 0)
		{
			perror ("listen");
			exit (EXIT_FAILURE);
		}

	printf("Initializing descriptors\n");
	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	while (1)
		{
			/* Block until input arrives on one or more active sockets. */
			printf("Waiting for input\n");
			read_fd_set = active_fd_set;
			if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
				{
					perror ("select");
					exit (EXIT_FAILURE);
				}

			/* Service all the sockets with input pending. */
			for (i = 0; i < FD_SETSIZE; ++i)
				if (FD_ISSET (i, &read_fd_set))
					{
						if (i == sock)
							{
								/* Connection request on original socket. */
								int new;
								size = sizeof (clientname);
								new = accept (sock,
															(struct sockaddr *) &clientname,
															&size);
								if (new < 0)
									{
										perror ("accept");
										exit (EXIT_FAILURE);
									}
								FD_SET (new, &active_fd_set);
							}
						else
							{
								/* Data arriving on an already-connected socket. */
								if (read_from_client (i) < 0)
									{
										close (i);
										FD_CLR (i, &active_fd_set);
									}
							}
					}
		}
}