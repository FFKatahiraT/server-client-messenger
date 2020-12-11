# server-client-messenger
This program is the solution for the next task:
Purpose: to develop server and client parts for organizing an instant messaging system.
Description: The server receives messages from clients and transmits them to the addressee.
When connecting to the server, clients provide their name (string).
The client remains connected to the server until the system is shut down.

The format of the message from the client can be as follows:
"Destination_name: message_text".

The format of the message from the server can be as follows:
"Sender_name: message_text".

programs marked as file_tr refers to:
Purpose: to develop a server for providing text files. Develop also a client.
Number of students: 2 (one creates the server, the other creates the client).
Description: The client connects to the server via a streaming connection (TCP / IP) and requests the contents of the required text file with the file name and maximum text length L (in bytes).
The server finds the specified file and transfers its contents to the client, but no more than L bytes.
The connection to the client remains until the quit command is received from the client.
If the specified file cannot be opened, the server sends an error message to the client.
