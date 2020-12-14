#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <limits.h>
#include <math.h>


int main(int argc , char *argv[])
{
    int PORT = 8941, max_clients = 8, len_message = 1024;
    int master_socket , addrlen , new_socket , client_socket[max_clients]  , activity, i , valread , sd;
	int max_sd, fd_stdin, data_lenght;
    struct sockaddr_in address;
    char buffer[64], name[max_clients][8], name_temp[8], command[5], *parsed_buffer, message[len_message];  //data size
    char doc_name[32], *endptr = "";
    const char *errstr;

    fd_set readfds;
    
    FILE *fp;
    
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
     
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORT );
     
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
	
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    
    fd_stdin = fileno(stdin);

    for(i=0; i<(log(max_clients)/log(2)+1); i++){
        fork();
    }
    i=0;
	while(1) 
    {
        ready:
        FD_ZERO(&readfds);
 
        FD_SET(master_socket, &readfds);
        FD_SET(fd_stdin, &readfds);
        fflush(stdout);
        max_sd = master_socket;
		
        for ( i = 0 ; i < max_clients ; i++) 
        {
            sd = client_socket[i];
            
			if(sd > 0)
				FD_SET( sd , &readfds);
            
            if(sd > max_sd)
				max_sd = sd;
        }
        
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        
        if(FD_ISSET(fd_stdin, &readfds)){
            read(fd_stdin, command, 5);
            if (strcmp(command, "quit")==10){
                exit(0); 
            }
        }
        if ((activity < 0) &&( errno!=EINTR)) 
        {
            printf("select error");
        }
         
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
         
            printf("New connection , socket is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            if( recv(new_socket , name_temp , 8 , 0) < 0)
            {
                puts("recv failed");
                break;
            }
              
            for (i = 0; i < max_clients; i++) 
            {
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    strcpy(name[i], name_temp);
                    printf("Adding to list of sockets as %d, name %s\n" , i, name_temp);
					break;
                }
            }
        }
         
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
            if (FD_ISSET( sd , &readfds)) 
            {
                if ((recv( sd , buffer, sizeof(buffer), 0)) <= 0)
                {
                    //getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    //printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     
                    close( sd );
                    client_socket[i] = 0;
                }
                else{
                    parsed_buffer = strtok(buffer," :");
                    if(sizeof(parsed_buffer)>32){
                        write(sd, "Use names up to 31 character", 29);
                        goto ready;
                    }
                    strcpy(doc_name, parsed_buffer);
                    parsed_buffer = strtok(NULL, " ");
                    if(!parsed_buffer){
                        write(sd, "Data lenght is empty", 21);
                        goto ready;
                    }
                    data_lenght = strtouq(parsed_buffer, &endptr, 10);
                    if(endptr[0] == '\0'){
                        write(sd, "Data lenght is not int", 23);
                        goto ready;
                    }
                    if(data_lenght>1024){
                        write(sd, "data_lenght should be less than 1024 bytes", 42);
                        goto ready;
                    }
                    fp = fopen(doc_name, "r");
                    if (fp == NULL){
                        write(sd, "There is no such document", 26);
                        goto ready;
                    }
                    if (fread(message, data_lenght, 1, fp)<=0){
                        write(sd, "failed to read text file", 20);
                        goto ready;
                    }
                    fclose(fp);
                    write(sd , message , strlen(message)+1);
                    memset(buffer, 0, sizeof(buffer));
                    memset(message, 0, sizeof(message));
                }
            }
        }
    }
     
    return 0;
}