// #include <netdb.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/time.h>

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "interface.h"

// #define SERVER_PORT     3005
// #define BUFFER_LENGTH    250
// #define FALSE              0  
// void main() {
//   int    sd=-1, sd2=-1;
//   int    rc, length, on=1;
//   char   buffer[BUFFER_LENGTH];
//   fd_set read_fd;
//   struct timeval timeout;
//   struct sockaddr_in serveraddr;
//   char *servIP="172.31.13.219";  
// do    {
//       sd = socket(AF_INET, SOCK_STREAM, 0);
//       // test error: sd < 0)      
//       memset(&serveraddr, 0, sizeof(serveraddr));
//       serveraddr.sin_family      = AF_INET;
//       serveraddr.sin_port        = htons(SERVER_PORT);
//       serveraddr.sin_addr.s_addr = htonl(servIP);
//       rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
//       // test error rc < 0
//       rc = listen(sd, 10);
//       // test error rc< 0
//       printf("Ready for client connect().\n");
//       sd2 = accept(sd, NULL, NULL);
//       // test error sd2 < 0
//       timeout.tv_sec  = 30;
//       timeout.tv_usec = 0;
//       FD_ZERO(&read_fd);
//       FD_SET(sd2, &read_fd);
//       rc = select(sd2+1, &read_fd, NULL, NULL, &timeout);
//       // test error rc < 0
//       length = BUFFER_LENGTH;
 
//       rc = recv(sd2, buffer, sizeof(buffer), 0);
//       // test error rc < 0 or rc == 0 or   rc < sizeof(buffer
//       rc = send(sd2, buffer, sizeof(buffer), 0);
//       // test error rc < 0
//   } while (FALSE);
//   if (sd != -1)
//       close(sd);
//   if (sd2 != -1)
//       close(sd2);
// }

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAX 256
#define PORT 8080
#define SA struct sockaddr

// Function designed for chat between client and server.
void func(int connfd)
{
	char buff[MAX];
	int n;
	// infinite loop for chat
	for (;;) {
		bzero(buff, MAX);

		// read the message from client and copy it in buffer
		read(connfd, buff, sizeof(buff));
		// print buffer which contains the client contents
		printf("From client: %s\t To client : ", buff);
		bzero(buff, MAX);
		n = 0;
		// copy server message in the buffer
		while ((buff[n++] = getchar()) != '\n')
			;

		// and send that buffer to client
		write(connfd, buff, sizeof(buff));

		// if msg contains "Exit" then server exit and chat ended.
		if (strncmp("exit", buff, 4) == 0) {
			printf("Server Exit...\n");
			break;
		}
	}
}

// Driver function
int main()
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("172.31.13.219");
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

	// Accept the data packet from client and verification
	connfd = accept(sockfd, (SA*)&cli, &len);
	if (connfd < 0) {
		printf("server accept failed...\n");
		exit(0);
	}
	else
		printf("server accept the client...\n");

	// Function for chatting between client and server
	func(connfd);

	// After chatting close the socket
	close(sockfd);
}
