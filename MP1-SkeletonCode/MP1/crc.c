#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "interface.h"

#define SA struct sockaddr
#define STDIN 0


/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port,int sockfd);

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    //display_title();g
    
	while (1) {
		
		display_title();
	
		int sockfd = connect_to(argv[1], atoi(argv[2]));
    
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0) {
			
			if (reply.status == SUCCESS){
				printf("Now you are in the chatmode\n");
				process_chatmode(argv[1], reply.port, sockfd);
			}
		}
	
		close(sockfd);
    }

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

    // below is just dummy code for compilation, remove it.
	// int sockfd = -1;
	// return sockfd;
	
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	// socket create and varification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		//printf("socket creation failed...\n");
		exit(0);
	}
	else
		//printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(host);
	servaddr.sin_port = htons(port);

	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		//printf("connection with the server failed...\n");
		exit(0);
	}
	else
		//printf("connected to the server..\n");
	
	
	return sockfd;
}

/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------
	int i;
	char buffer[256];
	char keyword[256];
	char message[256];
	
	
	memset(buffer, '\0', MAX_DATA);
	memset(keyword, '\0', MAX_DATA);
	memset(message, '\0', MAX_DATA);
	
	//printf("%s",command);
	bzero(buffer,sizeof(buffer));
	
	strcpy(buffer, command);
	if (!strncmp(buffer,"join",4)){
		for (int i = 0; i < 4; i ++){
			buffer[i] = toupper((unsigned char)buffer[i]);
		}
	}
	if (!strncmp(buffer,"create",6)){
		for (int i = 0; i < 6; i ++){
			buffer[i] = toupper((unsigned char)buffer[i]);
		}
	}
	if (!strncmp(buffer,"list",4)){
		for (int i = 0; i < 4; i ++){
			buffer[i] = toupper((unsigned char)buffer[i]);
		}
	}
	if (!strncmp(buffer,"delete",6)){
		for (int i = 0; i < 6; i ++){
			buffer[i] = toupper((unsigned char)buffer[i]);
		}
	}
	//buffer[255] = '\n';
	write(sockfd , buffer , sizeof(buffer));
	bzero(buffer, sizeof(buffer));
	memset(buffer, '\0', MAX_DATA);
	read(sockfd, buffer, sizeof(buffer));
	//printf("response is %s\n",buffer);
	//read(sockfd, buffer, sizeof(buffer));
	//read(sockfd, buffer, sizeof(buffer));
	for (i = 0; i < sizeof(buffer);i++){
		if (buffer[i] == ' '){
			memcpy(keyword,&buffer[0],i);
			memcpy(message, &buffer[i+1],sizeof(buffer)-i-1);
			break;
		}
	}
	//printf("keyword is %s reply is %s\n",keyword,message);
	if (!(strcmp(keyword,"CREATE"))){
		struct Reply reply;
		int val = atoi(message);
		if (val == 1){
			//printf("SUCCESS \n");
			reply.status = SUCCESS;
		}
		else{
			//printf("NO SUCCESS \n");
			reply.status = FAILURE_ALREADY_EXISTS;
		}
		return reply;
	}
	
	if (!(strcmp(keyword,"JOIN"))){
		struct Reply reply;
		char * token = strtok(message, " ");
	   // loop through the string to extract all other tokens
	   int counter = 0;
	   while( token != NULL ) {
	      //printf( "token %s\n", token ); //printing each token
	      
	      if (counter == 0){
	      	int val = atoi(token);
	      	if (val == 1){
				//printf("SUCCESS \n");
				reply.status = SUCCESS;
			}
			else{
				//printf("NO SUCCESS \n");
				reply.status = FAILURE_NOT_EXISTS;
			}
	      }
	      if (counter == 1){
	      	int members = atoi(token);
	      	//printf("members: %d",members);
	      	reply.num_member = members;
	      }
	      if (counter == 2){
	      	int port = atoi(token);
	      	//printf("port: %d",port);
	      	reply.port = port;
	      }
	      token = strtok(NULL, " ");
	      counter = counter+1;
	   }
	   return reply;
	}
	if (!(strcmp(keyword,"DELETE"))){
		struct Reply reply;
		int val = atoi(message);
		if (val == 1){
			//printf("SUCCESS \n");
			reply.status = SUCCESS;
		}
		else{
			//printf("NO SUCCESS \n");
			reply.status = FAILURE_NOT_EXISTS;
		}
		return reply;
		
	}
	
	if (!(strcmp(keyword,"LIST"))){
		struct Reply reply;
		//int val = atoi(message);
		if (strcmp(message,"0")){
			//printf("SUCCESS \n");
			reply.status = SUCCESS;
			strcpy(reply.list_room,message);
		}
		else{
			//printf("NO SUCCESS \n");
			reply.status = SUCCESS;
			strcpy(reply.list_room,"empty");
		}
		return reply;
		
	}
	
	

	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:

// send back string separated by \n
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------

	// REMOVE below code and write your own Reply.
	// struct Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = 5;
	// reply.port = 1024;
	// return reply;
}

/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port, int sockfd)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	//int sockfd = connect_to("c",port);
	//printf("sockfd %d",sockfd);
	char buff[MAX_DATA];
	//char buff2[MAX];
	int n;
	fd_set readfds;
	//FD_ZERO(&readfds);
	// FD_SET(STDIN, &readfds); 
	// 	FD_SET( sockfd , &readfds);
		//printf("%d",sockfd);
	while(1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN, &readfds); 
		FD_SET( sockfd , &readfds);
		//printf("waitingSock");
		int activity = select( 4, &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			//printf("select error");
		}
		if (FD_ISSET(sockfd,&readfds)){
			//printf("sockif");
			bzero(buff, sizeof(buff));
			read(sockfd, buff, sizeof(buff));
			printf("> %s", buff);
			if (!strcmp(buff,"Warning, the chatting room is going to be closed...\r\n")){
				break;
			}
			//display_message(buff);
			//bzero(buff, sizeof(buff));
			
		}
		//printf("waitinginput");
		if (FD_ISSET(STDIN,&readfds)){
			//printf("writing");
			//while ((buff[n++] = getchar()) != '\n'){
		// // 	// read(sockfd, buff2, sizeof(buff2));
		// // 	// if (sizeof(buff2)!= 0){
		// // 	// 	printf("From Server inside : %s", buff2);	
			// }
			bzero(buff, sizeof(buff));
			read(STDIN, buff, sizeof(buff));
			// char Q[256] = "Q";
			// if (!strcmp(buff,Q)){
			// 	break;
			// }
			char* message;
			//get_message(message, MAX_DATA);
			//strcpy(buff,message);
			write(sockfd, buff, sizeof(buff));
			//bzero(buff, sizeof(buff));
		}
	}
	// char* message;
	// pthread_t thread_id;
	// pthread_t thread_id2;
 //   printf("Before Thread\n");
 //   pthread_create(&thread_id, NULL, myThreadFun, NULL);
 //   pthread_join(thread_id, NULL);
 //   pthread_join(thread_id2, NULL);
 //   printf("After Thread\n");
 //   exit(0);
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

