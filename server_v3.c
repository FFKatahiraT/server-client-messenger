#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

int main(int argc , char *argv[])
{
    //int opt = 1;
    int PORT = 8925, max_clients = 30;
    int master_socket , addrlen , new_socket , client_socket[max_clients]  , activity, i , valread , sd, search_cli;
	int max_sd, fd_stdin;
    struct sockaddr_in address;
    char buffer[1024], name[max_clients][8], name_temp[8], * destination, command[5], * message;  //data size
     
    //set up select()
    fd_set readfds;
     
    //a message
    // char *message = "Connected to server \r\n";
 
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
     
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
     
    //bind the socket to localhost port 8888
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
    
    fd_stdin = fileno(stdin);

	while(1) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
 
        //add master socket to set
        FD_SET(master_socket, &readfds);
        FD_SET(fd_stdin, &readfds);
        //printf("Enter command: ");
        fflush(stdout);
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
        
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        
        if(FD_ISSET(fd_stdin, &readfds)){
            read(fd_stdin, command, 5);
            /* process command, maybe by sscanf */
            if (strcmp(command, "quit")){
                break; /* to terminate loop, since I don't process anything */  
            }
        }
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
         
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
         
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //recieve client's name
            if( recv(new_socket , name_temp , 2000 , 0) < 0)
            {
                puts("recv failed");
                break;
            }
              
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
				if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    strcpy(name[i], name_temp);
                    printf("Adding to list of sockets as %d, name %s\n" , i, name_temp);
					break;
                }
            }
        }
         
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
            if (FD_ISSET( sd , &readfds)) 
            {
                //Check if it was for closing , and also recieve the incoming message
                if ((recv( sd , buffer, sizeof(buffer), 0)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
                 
                //Echo back the message that came in
                else
                {
                    printf("before strtoc %s\n", buffer);
                    destination = strtok(buffer," :");
                    printf("after destination %s\n", buffer);
                    for( search_cli = 0 ; search_cli < sizeof(client_socket); search_cli += 1 ){  //find a receiver
                        //printf("%d, name: %s\n", search_cli, name[search_cli]);
                        if(strcmp(destination, name[search_cli]) == 0){
                            // printf("%s\n", "MATCH");
                            // printf("dest %s\n", destination);
                            // printf("name %s\n", name[search_cli]); 
                            break;
                        }
                    }
                    if(search_cli==sizeof(client_socket) || client_socket[search_cli]==0){
                        write(sd , "There is no such receiver" , 26);
                    }
                    else{
                        message = strtok(buffer, " :");
                        //set the string terminating NULL byte on the end of the data read
                        //message[sizeof(message)] = '\0';
                        //printf("socket %d\n", client_socket[search_cli]);
                        write(client_socket[search_cli] , message , strlen(message));
                        printf("after send %s\n", buffer);
                        memset(buffer, 0, sizeof(buffer));
                        destination = strtok(NULL, " ");
                        message = strtok(NULL, " ");
                    }
                }
            }
        }
    }
     
    return 0;
}