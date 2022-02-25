// #define SERVER_PORT     3005
// #define BUFFER_LENGTH    250
// #define FALSE              0
// #define SERVER_NAME     "ServerHostName"
// void main(int argc, char *argv[]) {
//   int    sd=-1, rc, bytesReceived;
//   char   buffer[BUFFER_LENGTH];
//   char   server[NETDB_MAX_HOST_NAME_LENGTH];
//   struct sockaddr_in serveraddr;
//   struct hostent *hostp;
//   do   {
//       sd = socket(AF_INET, SOCK_STREAM, 0);
//       //test error sd < 0
//       if (argc > 1)  strcpy(server, argv[1]);
//       else  strcpy(server, SERVER_NAME);
//       memset(&serveraddr, 0, sizeof(serveraddr));
//       serveraddr.sin_family      = AF_INET;
//       serveraddr.sin_port        = htons(SERVER_PORT);
//       serveraddr.sin_addr.s_addr = inet_addr(server);
//       if (serveraddr.sin_addr.s_addr == (unsigned long)INADDR_NONE)      {
//          hostp = gethostbyname(server);
//          if (hostp == (struct hostent *)NULL) {
//             printf("Host not found --> ");
//             break;
//          }

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#define MAX 256
#define PORT 8080
#define SA struct sockaddr
#define STDIN 0
void func(int sockfd)
{
	printf("%d",sockfd);
	char buff[MAX];
	//char buff2[MAX];
	int n;
	fd_set readfds;
	//FD_ZERO(&readfds);
	// FD_SET(STDIN, &readfds); 
	// 	FD_SET( sockfd , &readfds);
		printf("%d",sockfd);
	while(1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN, &readfds); 
		FD_SET( sockfd , &readfds);
		printf("waitingSock");
		int activity = select( 4, &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
		if (FD_ISSET(sockfd,&readfds)){
			printf("sockif");
			bzero(buff, sizeof(buff));
			read(sockfd, buff, sizeof(buff));
			printf("From Server : %s", buff);
			//bzero(buff, sizeof(buff));
			
		}
		printf("waitinginput");
		if (FD_ISSET(STDIN,&readfds)){
			printf("writing");
			//while ((buff[n++] = getchar()) != '\n'){
		// // 	// read(sockfd, buff2, sizeof(buff2));
		// // 	// if (sizeof(buff2)!= 0){
		// // 	// 	printf("From Server inside : %s", buff2);	
			// }
			bzero(buff, sizeof(buff));
			read(STDIN, buff, sizeof(buff));
			write(sockfd, buff, sizeof(buff));
			//bzero(buff, sizeof(buff));
		}
		// if ((strncmp(buff, "exit", 4)) == 0) {
		// 	printf("Client Exit...\n");
		// 	break;
		// }
		// read(sockfd, buff, sizeof(buff));
		// printf("From Server : %s", buff);
		// bzero(buff, sizeof(buff));
		// printf("Enter the string : ");
		// n = 0;
		// while ((buff[n++] = getchar()) != '\n'){
		// 	// read(sockfd, buff2, sizeof(buff2));
		// 	// if (sizeof(buff2)!= 0){
		// 	// 	printf("From Server inside : %s", buff2);	
		// 	// }
			
		// }
		// write(sockfd, buff, sizeof(buff));
		// //bzero(buff, sizeof(buff));
		// //read(sockfd, buff, sizeof(buff));
		// //printf("From Server : %s", buff);
		// if ((strncmp(buff, "exit", 4)) == 0) {
		// 	printf("Client Exit...\n");
		// 	break;
		// }
	}
}

int main()
{
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	// socket create and varification
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

	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
		exit(0);
	}
	else
		printf("connected to the server..\n");

	// function for chat
	func(sockfd);

	// close the socket
	close(sockfd);
}
