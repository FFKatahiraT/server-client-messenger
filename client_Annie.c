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
	int sock;
	struct sockaddr_in server;
	char server_reply[2000];
	
	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8920);

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}
	
	char name[8] = "Annie";
	send(sock , name, 8 , 0);
	puts("Connected\n");

	while(1)
	{
		char receiver[16];
		char message[1024];
		//recv(sock , server_reply , 2000 , 0);
		// printf("Receiver : ");
		// scanf("%s" , receiver);
		printf("Enter message : ");
		scanf("%s" , message);

		//Send some data
		if( send(sock , message , strlen(message) , 0) >= 0)
		{
			//send(sock , message , strlen(message) , 0);
		}
		else{
			puts("Send failed");
			return 1;			
		}
		
		//Receive a reply from the server
		if( recv(sock , server_reply , 2000 , 0) < 0)
		{
			puts("recv failed");
			break;
		}
		
		puts("Server reply :");
		puts(server_reply);
	}
	
	close(sock);
	return 0;
}