/**
* @file client.c
* @brief This file contains the code for the client.
* The client read from command line arguments the client's name, 
* the inventories will be ask, from the server, and the server host.
* The format must be "./client -n <name> -i <inventory_file_name> <server_host>".
* First read the inventories and save it in a inventory array. Then send
* a request to the server with his name and the inventories. If the server's
* answer is "OK", the client added from the server in a table and wait until 
* its full. While the client wait received from the server the message
* "Please wait". When the table is full, client received the message "START".
* Next creates two threads. The first one is for read from the server and 
* the second one for write, parallel. This two function with the server 
* constitute the chat room. 
*/


#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "consts.h"
#include <unistd.h>
#include <sys/socket.h> /* basic socket definitions */
#include <sys/types.h> /* basic system data types */
#include <stdlib.h>
#include <netdb.h> /* for hostent structure */
#include "inventory.h"
#include "parsing.h"
#include <pthread.h>

void* writeHandler( void* args );
void* readHandler( void * args );

char serv_answer[256]; /**< String buffer used to save the server's answers */
int sock = -1; /**< The socket descriptor */

int main(int argc, char ** argv)
{
	pthread_t tid[2];
	struct sockaddr_in servaddr;
	struct hostent *server;
	
	char toString[512] = "\0";
	char tempString[512] = "\0";
	
	char name[20];
	inventory* inv;	
	int inv_size;
	
	/* check for command line arguments */
	if ( argc!=6 || strcmp(argv[1],"-n") || strcmp(argv[3],"-i") )
	{
		fprintf(stderr, "Error: %s -n <name> -i <inventory> <server_host> \n", argv[0]);
		exit(1);
	}

	strcpy(name, argv[2]);

	/* read content from client inventory file */
	inv = readInventory(argv[4], &inv_size);

	printf("+----------------------------------------------------------------------------+\n");
	printf("|   _____          __  __ ______    _____ _      _____ ______ _   _ _______  |\n");
	printf("|  / ____|   /\\   |  \\/  |  ____|  / ____| |    |_   _|  ____| \\ | |__   __| |\n");
 	printf("| | |  __   /  \\  | \\  / | |__    | |    | |      | | | |__  |  \\| |  | |    |\n");
 	printf("| | | |_ | / /\\ \\ | |\\/| |  __|   | |    | |      | | |  __| | . ` |  | |    |\n");
 	printf("| | |__| |/ ____ \\| |  | | |____  | |____| |____ _| |_| |____| |\\  |  | |    |\n");
  	printf("|  \\_____/_/    \\_\\_|  |_|______|  \\_____|______|_____|______|_| \\_|  |_|    |\n");
  	printf("+----------------------------------------------------------------------------+\n");

	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock <0)
	{
		perror("Error: ");
		exit(1);
	}

	/* connect to server */
	server = gethostbyname(argv[5]);	
	if ( server == NULL )
	{
		perror("Error: ");
		exit(1);
	}
	/* Define the name of this socket. */
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORTNUM);
	
	memcpy(&servaddr.sin_addr, server->h_addr_list[0], server->h_length);
	if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
	{
		perror("Error:" );
		return -1;
	}

	/* use two function to convert all in one string */
	strcpy (toString,name);
	inventoryArrayToString(inv, tempString, inv_size);
	addToSend (toString,tempString);

	/* send string to server */
	write (sock , toString , strlen(toString)+1);
	
	/* print string where sended */
	printf("%s \n", toString);

	/* read answer from server */
	if (read (sock, serv_answer, sizeof(serv_answer)) == 0 ) 
	{
		fprintf(stderr, "Server close connection! \n" );
		close( sock );
		exit(1);
	}

	/* if the inventory is not correct then the client terminate */
	if ( strcmp(serv_answer,"OK") !=0) 
	{
		printf("%s\n",serv_answer);
		exit(1);
	}

	/*print server's answer*/
	printf("%s\n",serv_answer);

	while( strcmp(serv_answer, "START") != 0 )
	{
		/*read answer from server*/
		if (read (sock, serv_answer, sizeof(serv_answer)) == 0 ) 
		{
			fprintf(stderr, "Server close connection! \n" );
			exit(1);
		}
		/*print server's answer*/
		printf("Server's answer  : %s\n",serv_answer);
	}
	
	/* create read & write threads */
	int err;
	err = pthread_create(&tid[1], NULL, writeHandler, NULL);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));

    err = pthread_create(&tid[0], NULL, readHandler, NULL);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));	

	pthread_join(tid[1], NULL);
	pthread_join(tid[0], NULL);
	
	/* close socket */
	 close(sock);

	return 0;

}

/**
* This function continue read the message that 
* come from the server.
* @param args Unused parameter
*/
void* readHandler( void* args )
{
	while( TRUE )
	{
		/*read answer from server*/
		if (read (sock, serv_answer, sizeof(serv_answer)) == 0 ) 
		{
			fprintf(stderr, "Server close connection! \n" );
			close( sock );
			exit(1);
		}
		/*print server's answer*/
		printf(" - %s\n",serv_answer);
	}	
} 

/**
* This function continue write the message that 
* the user type to the server.
* @param args Unused parameter
*/
void* writeHandler( void* args )
{
	while( TRUE )
	{
		char buf_send[50];
		fgets(buf_send,49,stdin);
		if(write (sock , buf_send , strlen(buf_send)+1) < 0)
		{
			fprintf(stderr, "Server close connection! \n" );
			close( sock );
			exit(1);
		}
	}
}