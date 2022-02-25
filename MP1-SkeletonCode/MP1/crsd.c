//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
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
	
#define TRUE 1
#define FALSE 0
#define PORT 8080
#define MAX 256
#define MAX_ROOMS 64
#define MAX_CLIENTS 256
	
int main(int argc , char *argv[])
{
	int port = atoi(argv[1]);
	int opt = TRUE;
	int master_socket , addrlen , new_socket , 
		max_clients = MAX_CLIENTS  , activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;
	int client_socket[max_clients];	
	char buffer[MAX]; //data buffer of 1K
	char rooms [MAX_ROOMS][MAX];
	int socket_rooms[max_clients];
	//set of socket descriptors
	fd_set readfds;
	freopen("output.txt", "a+", stdout); 
	//set all rooms to empty
	for (i = 0; i < MAX_ROOMS; i ++){
		memset(rooms[i], '\0', MAX);
	}
		
	//a message
	char *message = "ECHO Daemon v1.0 \r\n";
	
	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
		socket_rooms[i] = -1;
	}
		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
		sizeof(opt)) < 0 )
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
		
	//bind the socket to localhost port 8080
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);
		
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");
		
	while(TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);
	
		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;
			
		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++)
		{
			//socket descriptor
			sd = client_socket[i];
				
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);
				
			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}
	
		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		printf("activity %d \n",activity);
	
		if ((activity < 0) && (errno!=EINTR))
		{
			printf("select error");
		}
			
		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
		
			//send new connection greeting message
			// if( send(new_socket, message, strlen(message), 0) != strlen(message) )
			// {
			// 	perror("send");
			// }
				
			// puts("Welcome message sent successfully");
				
			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if( client_socket[i] == 0 )
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d, fd is %d\n" , i,client_socket[i]);
						
					break;
				}
			}
		}
			
		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];
			//printf("for loop socket sockfd %d and i is %d \n",sd,i);
				
			if (FD_ISSET( sd , &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valread = read( sd , buffer, sizeof(buffer))) == 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , \
						(socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" ,
						inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						
					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
					socket_rooms[i] = -1;
				}
				
				//Echo back the message that came in
				else
				{
					//printf("else socket sockfd %d and i is %d \n",sd,i);
					//set the string terminating NULL byte on the end
					//of the data 
					//char command[];
					char subtext [MAX];
					char command [MAX];
					char commands  [4][10] = {"CREATE","JOIN","DELETE","LIST"};
					char reply [MAX];
					int success = 0;
					int repeat = 0;
					int members;
					int j;
					
					memset(reply, '\0', MAX);
					memset(command, '\0', MAX);
					memset(subtext, '\0', MAX);
				
					//create
					int ret = strncmp(buffer,commands[0],strlen(commands[0]));
					if (ret == 0){
						printf("ret %s",commands[0]);
						strncpy(command,commands[0],strlen(commands[0]));
						strncpy(reply,commands[0],strlen(commands[0]));
						//find the end of string character
						for (j = 0; j < sizeof(buffer);j++){
							if (buffer[j] == '\0'){
								printf("eol %d \n",j);
								break;
							}
						}
						//use that index  and the index of the end of command +1 to find room name
						memcpy(subtext,&buffer[strlen(commands[0])+1],MAX-strlen(commands[0])-1);
						printf("subtext %s \n",subtext);
						//check if room exists
						for (j = 0; j < MAX_ROOMS; j++){
							ret = strcmp(rooms[j],subtext);
							if (ret == 0){
								printf("repeat found \n");
								repeat = 1;
								break;
							}
						}
						//if room doesnt exist, we can add it
						
						if (repeat == 0){
							for (j = 0; j < MAX_ROOMS;j++ ){
								if (rooms[j][0] == '\0'){
									strncpy(rooms[j],subtext,sizeof(subtext));
									success = 1;
									break;
								}
							}
						
							printf("index: %d roomName %s \n",j,rooms[j]);
						}
						
						char numbuf [256];
						//numbuf[0] = '\0';
						//char num [256];
						//num[0]
					    sprintf(numbuf, "%d", success);
					    //strcpy(numbuf,num);
					    printf("reply is %s",reply);
					    strcat(reply," ");
					    strcat(reply,numbuf);
				
					
						
						
						send(sd , reply , sizeof(reply),0);
					}
					
					
					//join
					int roomNo = -1;
					ret = strncmp(buffer,commands[1],strlen(commands[1]));
					if (ret == 0){
						printf("ret %s\n",commands[1]);
						strncpy(command,commands[1],strlen(commands[1]));
						strncpy(reply,commands[1],strlen(commands[1]));
						for (j = 0; j < sizeof(buffer);j++){
							if (buffer[j] == '\0'){
								printf("eol %d \n",j);
								break;
							}
						}
						//get room to join
						memcpy(subtext,&buffer[strlen(commands[1])+1],MAX-strlen(commands[1])-1);
						printf("subtext k%sk \n",subtext);
						for (j = 0; j < MAX_ROOMS; j++){
							printf("rooms j k%sk",rooms[j]);
							ret = strcmp(rooms[j],subtext);
							if (ret == 0){
								printf("room is present \n");
								//
								roomNo = j;
								break;
							}
						}
						printf("JOIN index: %d roomName %s \n",j,rooms[j]);
						for (j = 0; j <max_clients; j++){
						if (sd == client_socket[j] && roomNo != -1){
								socket_rooms[j] = roomNo;
							}
						}
						
						
						int members = 0;
						for (j = 0; j <max_clients; j++){
						if (socket_rooms[j] == roomNo && roomNo != -1){
								members = members+1;
							}
						}
						
						
						if (roomNo >= 0){
							success = 1;
							
						}
						
						//append info to reply
						char numbuf [256];
						//append success
					    sprintf(numbuf, "%d", success);
					    //strcpy(numbuf,num);
					    printf("reply is %s",reply);
					    strcat(reply," ");
					    strcat(reply,numbuf);
					    
					    //append members
					    numbuf[0] = '\0';
					    sprintf(numbuf, "%d", members);
					    strcat(reply," ");
					    strcat(reply,numbuf);
					    
					    //append port
					    numbuf[0] = '\0';
					    sprintf(numbuf, "%d", ntohs(address.sin_port));
					    strcat(reply," ");
					    strcat(reply,numbuf);
					    
					    send(sd , reply , sizeof(reply),0);
					}
					
					//delete
					ret = strncmp(buffer,commands[2],strlen(commands[2]));
					int roomId = -1;
					int delsuccess = 0;
					if (ret == 0){
						printf("ret %s\n",commands[2]);
						strncpy(command,commands[2],strlen(commands[2]));
						strncpy(reply,commands[2],strlen(commands[2]));
						
						//get command message
						memcpy(subtext,&buffer[strlen(commands[2])+1],MAX-strlen(commands[2])-1);
						for (int j = 0; j < MAX_ROOMS; j++){
							if (!strcmp(subtext,rooms[j])){
								roomId = j;
								memset(rooms[j], '\0', MAX);
								break;
							}
						}
						if (roomId != -1){
							for (int j = 0; j < max_clients; j++){
								if (roomId == socket_rooms[j]){
									printf("room found to be deleted is %d sd is %d",roomId, client_socket[j]);
									socket_rooms[j] = -1;
									char deleteMessage[256] = "Warning, the chatting room is going to be closed...\r\n";
									printf("sending %s to %d \n", deleteMessage,client_socket[j]);
									send(client_socket[j] , deleteMessage , sizeof(deleteMessage) , 0 );
								}
							}
						}
						if (roomId >= 0){
							delsuccess = 1;
							
						}
						
						//append info to reply
						char numbuf [256];
						memset(numbuf,'\0',MAX);
						//append success
					    sprintf(numbuf, "%d", delsuccess);
					    //strcpy(numbuf,num);
					    printf("reply is %s",reply);
					    strcat(reply," ");
					    strcat(reply,numbuf);
						send(sd , reply , sizeof(reply) , 0 );
						
					}
					
					//list
					ret = strncmp(buffer,commands[3],strlen(commands[3]));
					//int roomId = -1;
					int listsuccess = 0;
					if (ret == 0){
						printf("ret %s\n",commands[3]);
						strncpy(command,commands[3],strlen(commands[3]));
						strncpy(reply,commands[3],strlen(commands[3]));
						char list[256];
						memset(list,'\0',MAX);
						for (int j = 0; j < MAX_ROOMS; j++){
							if (rooms[j][0]!= '\0'){
								strcat(list,rooms[j]);
								strcat(list,",");
							}
						}
						strcat(reply," ");
						if (list[0] == '\0'){
							strcat(reply,"0");
						}
						else{
							strcat(reply,list);
						}
						send(sd , reply , sizeof(reply) , 0 );
					}
						
						
						
					
					
					
					
					//printf("final command %s",command);
					
					printf("received %s from %d \n",buffer,sd);
					
					int isCommand = 0;
					//int isSecondCommand = 0;
					char buffer2[MAX];
					//printf("comm len %s \n",strlen(command));
					if(command[0] != '\0'  && strncmp(command,"JOIN",strlen("JOIN"))){
						printf("waiting for more");
						read( sd , buffer2, sizeof(buffer2));
					}
					if (command[0] != '\0'){
						for (int j = 0; j < 4; j++){
							printf("comp %s %s \n",command,commands[j]);
							if (!strncmp(command,commands[j],strlen(commands[j]))){
								isCommand = 1;
								break;
							}
						}
					}
					if(!isCommand){
						printf("gonna send messaages");
						for (int j = 0; j < max_clients; j++){
							if (client_socket[j] == sd){
								roomNo = socket_rooms[j];
							} 
						}
						for (int j = 0; j < max_clients;j++){
							
							if (client_socket[j] != 0 && client_socket[j] != sd && socket_rooms[j] == roomNo){
								printf("sending %s to %d \n", buffer,client_socket[j]);
								send(client_socket[j] , buffer , sizeof(buffer) , 0 );
							}
						}
					}
						
						
					// int isCommand = 0;
					// int isSecondCommand = 0;
					// char buffer2[MAX];
					// for (j = 0; j < 4; j ++){
					// 	if (!strncmp(buffer,commands[i],strlen(commands[i]))){
					// 		isCommand = 1;
					// 		break;
					// 	}
					// }
					// if (isCommand){
					// 	if (select(sd+1, &readfds, NULL,NULL, 0)==1){
					// 		read( sd , buffer2, sizeof(buffer2));
					// 	}
						
					// 	printf("2received %s from %d \n",buffer2,sd);
					// 	strcpy(buffer,buffer2);
					// 	for (j = 0; j < 4; j ++){
					// 		if (!strncmp(buffer,commands[i],strlen(commands[i]))){
					// 			isSecondCommand = 1;
					// 			break;
					// 		}
					// 	}
					// 	if (isSecondCommand==0){
					// 		strcpy(buffer,buffer2);
					// 	}
					// }
					// if(!isSecondCommand){
					// 	printf("second is not command");
					// 	//buffer[valread] = '\0';
					// 	for (int j = 0; j < max_clients; j++){
					// 		if (client_socket[j] == sd){
					// 			roomNo = socket_rooms[j];
					// 		} 
					// 	}
					// 	for (int j = 0; j < max_clients;j++){
							
					// 		if (client_socket[j] != 0 && client_socket[j] != sd && socket_rooms[j] == roomNo){
					// 			printf("sending %s to %d \n", buffer,client_socket[j]);
					// 			send(client_socket[j] , buffer , sizeof(buffer) , 0 );
					// 		}
					// 	}
					// }
				}
				//bzero(buffer, sizeof(buffer));
			}
		}
	}
		
	return 0;
}
