/*
	C ECHO client example using sockets
*/
#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>

int main(int argc , char *argv[])
{
	int sock, fd_stdin;
	struct sockaddr_in server;
	char server_reply[2000], message[1024];;
	
	//set up select()
    fd_set readfds;
	
	fd_stdin = fileno(stdin);

	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8934);

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}
	
	char name[8] = "John";
	send(sock , name, 8 , 0);
	puts("Connected\n");

	while(1)
	{
		//clear the socket set
        FD_ZERO(&readfds);
 
        //add to set
        FD_SET(fd_stdin, &readfds);
        FD_SET( sock , &readfds);
        printf("%s", "Enter a message: ");
		fflush(stdout);
		
		select(sock + 1, &readfds, NULL, NULL, NULL);
	    
	    if(FD_ISSET(fd_stdin, &readfds)){  
	        read(fd_stdin, message, 1024);
			if(strcmp(message, "quit") == 0){
				break;
			}
			//Send some data
			if( send(sock , message , strlen(message) , 0) >= 0)
			{
				//send(sock , message , strlen(message) , 0);
			}
			else{
				puts("Send failed");
				return 1;			
			}
			memset(message, 0, sizeof(message));
		}
		
		if(FD_ISSET(sock, &readfds)){
		    //Check if it was for closing , and also recieve the incoming message
            if ((recv(sock , server_reply , 2000 , 0)) <= 0)
            {
                printf("Server disconnected");
                //Close the socket and mark as 0 in list for reuse
                break;
            }
            else{
            	puts("Server reply :");
				puts(server_reply);
            }
		}
	}
	
	close(sock);
	return 0;
}