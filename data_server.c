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
    //int opt = 1;
    int PORT = 8960, max_clients = 8, len_message = 1024;
    int master_socket , addrlen , new_socket , client_socket[max_clients]  , activity, i , valread , sd;
	int max_sd, fd_stdin, data_lenght;
    struct sockaddr_in address;
    char buffer[len_message], name[max_clients][8], name_temp[8], command[5], *parsed_buffer, message[len_message];  //data size
    char doc_name[64], clients_command[16], *endptr = "", data[len_message-9], str[128], full_data_lenght[16];
    const char *errstr;

    //set up select()
    fd_set readfds;
    //for file proccessing
    FILE *fp;
    
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

    for(i=0; i<(log(max_clients)/log(2)+1); i++){
        fork();
    }
    i=0;
	while(1) 
    {
        ready:
        //clear the socket set
        FD_ZERO(&readfds);
 
        //add master socket to set
        FD_SET(master_socket, &readfds);
        FD_SET(fd_stdin, &readfds);
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
            if (strcmp(command, "quit")==10){
                exit(0); 
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
            if( recv(new_socket , name_temp , 8 , 0) < 0)
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
                if ((recv( sd , buffer, sizeof(buffer), 0)) <= 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
                //PROCCESSING
                else{
                    printf("%s\n", "recieved msg");
                    parsed_buffer = strtok(buffer," :");
                    if(sizeof(parsed_buffer)>64 || !parsed_buffer){
                        write(sd, "Use names up to 64 bytes", 29);
                        goto ready;
                    }
                    strcpy(doc_name, parsed_buffer);
                    //printf("help %s\n", doc_name);
                    if(strcmp(doc_name, "help")==10){
                        strcpy(str, "read-->  file_name -r file_lenght(int)/full\n");
                        strcat(str, "write--> file_name -w your_text\n");
                        strcat(str, "delete--> file_name -del\n");
                        write(sd, str, 128);
                        memset(str, 0, sizeof(str));
                        goto ready;
                    }

                    parsed_buffer = strtok(NULL, " ");
                    if(!parsed_buffer){
                        goto ready;
                    }
                    //printf("%s\n", parsed_buffer);
                    //printf("sizeof %ld\n", sizeof(parsed_buffer));
                    if(sizeof(parsed_buffer)>16 || !parsed_buffer){
                        write(sd, "Invalid command, type help to get the list of commands", 55);
                        goto ready;
                    }
                    strcpy(clients_command, parsed_buffer);

                    //printf("%d\n", strcmp(clients_command,"-del"));
                    if(strcmp(clients_command,"-del")==0){
                        if(remove(doc_name)!=0){
                            write(sd, "Unable to delete data", 22);
                            goto ready;
                        }
                        printf("File deleted %s\n", doc_name);
                        write(sd, "File deleted successfully", 26);
                    }
                    parsed_buffer = strtok(NULL, "");
                    if(!parsed_buffer){
                        goto ready;
                    }
                    //printf("r %d\n", strcmp(clients_command, "-r"));
                    if(strcmp(clients_command, "-r")==0){    
                        //printf("Im Here rs %s\n", doc_name);
                        if(sizeof(parsed_buffer)>16){
                            write(sd, "comand for size", 16);
                            goto ready;                            
                        }
                        strcpy(full_data_lenght, parsed_buffer);
                        //printf("Full %d\n", strcmp(full_data_lenght, "full"));
                        fp = fopen(doc_name, "r");
                        if (fp == NULL){
                            write(sd, "There is no such document", 26);
                            goto ready;
                        }
                        if(strcmp(full_data_lenght, "full")==10){
                            while (fgets(str, 1024, fp) != NULL){
                                strcat(message, str);
                            }
                            fclose(fp);
                            memset(str,0,sizeof(str));
                        }
                        else{
                            data_lenght = strtouq(parsed_buffer, &endptr, 10);
                            if(endptr[0] == '\0'){
                                write(sd, "Data lenght is not int", 23);
                                goto ready;
                            }
                            if(data_lenght>1024){
                                write(sd, "data_lenght should be less than 1024 bytes", 42);
                                goto ready;
                            }
                            if (fread(message, data_lenght, 1, fp)<=0){
                                write(sd, "failed to read text file", 25);
                                goto ready;
                            }
                            fclose(fp);
                        }
                    }
                    //printf("w %d\n", strcmp(clients_command, "-w"));
                    if(strcmp(clients_command,"-w")==0){
                        //printf("Im Here w %s\n", doc_name);
                        fp = fopen(doc_name, "w");
                        if(fp == NULL){
                            write(sd, "Unable to create/write text file", 33);
                            goto ready;
                        }
                        strcpy(data, parsed_buffer);
                        fputs(data, fp);
                        fclose(fp);
                        strcpy(message, "File saved successfully");
                    }

                    write(sd , message , strlen(message)+1);
                    memset(buffer, 0, sizeof(buffer));
                    memset(message, 0, len_message);
                }
            }
        }
    }
     
    return 0;
}