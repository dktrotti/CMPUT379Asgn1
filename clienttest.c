//Source: http://www.gnu.org/software/libc/manual/html_node/Byte-Stream-Example.html

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

void write_to_server (int filedes, char* message) {
    int nbytes;

    nbytes = write (filedes, message, strlen(message) + 1);
    if (nbytes < 0) {
        perror ("write");
        exit (EXIT_FAILURE);
    }
}

void init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port) {
  struct hostent *hostinfo;

  name->sin_family = AF_INET;
  name->sin_port = htons (port);
  hostinfo = gethostbyname (hostname);
  if (hostinfo == NULL)
    {
      fprintf (stderr, "Unknown host %s.\n", hostname);
      exit (EXIT_FAILURE);
    }
  name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}


int main (int argc, char* argv[]) {
    if (argc != 3) {
        printf("Invalid number of arguments\n");
        exit(1);
    }
    char* serverhost = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in servername;

    /* Create the socket. */
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror ("socket (client)");
        exit (EXIT_FAILURE);
    }

    /* Connect to the server. */
    init_sockaddr (&servername, serverhost, port);
    if (0 > connect (sock,
            (struct sockaddr *) &servername,
            sizeof (servername)))
    {
        perror ("connect (client)");
        exit (EXIT_FAILURE);
    }

    /* Send data to the server. */
    sleep(5);
    write_to_server (sock, "cfg");
    char config[256];
    memset(config, 0, sizeof(config));
    read(sock, config, 256);
    //printf("Client got: %s\n", config);
    char * curLine = config;
    while(curLine) {
        char * nextLine = strchr(curLine, '\n');
        if (nextLine) *nextLine = '\0';  // temporarily terminate the current line
        printf("curLine=[%s]\n", curLine);
        if (nextLine) *nextLine = '\n';  // then restore newline-char, just to be tidy    
        curLine = nextLine ? (nextLine+1) : NULL;
    }
    write_to_server(sock, "Killed processname (PID 3312) after 120 seconds.\n");
    //sleep(5);
    close (sock);
    exit (EXIT_SUCCESS);
}