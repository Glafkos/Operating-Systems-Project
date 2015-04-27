/**
* @mainpage Multiplayer Online Game
*	This project is a game server for a multiplayer online 
*	game with specific number of players. Every new player
*	that connect to the server choose his starting inventory. 
*	When the number of players complete then start the game.
*	After the players start to play they can chat online via 
*	the server. The server-client communication is TCP and
*	send ASCII data each other. The server host table that
*	client join. To start a table the game must complete the
*	required number of players. Client to join in send a 
*	request with his name and the inventories that he want. 
*	If the inventory is correct the player join else close 
* 	the connection. The server's inventories is supplies for 
*	every table. The server to can handle more than one 
*	connections use posix threads to run the clients parallel. 
*/

/**
* @file server.c
* @brief This file contains the server's code. 
* The server, first, read from command line arguments the number of 
* players, the  table inventory, and the quota per player for each 
* table. The format must be "./server -p <num_of_player> -i 
* <game_inventory_file_name> -q <quota_per_player>". First read the 
* inventories and save it in a inventory array. Then create and open 
* the socket connection and wait for clients to connect. For every
* client that connect we save his socket descriptor in a array and we open
* a new thread to manage the communication with him. For each client first
* the server received him request and check it if is correct. If the client's
* request is not correct, server send to this client the message "Invalid 
* inventory" and close the connection. If client's request is correct then 
* book the inventory and add him to the current table. Next the client wait
* until to complete his table and start the game. 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "consts.h"
#include "inventory.h"
#include "parsing.h"
#include "sigHan.h"
#include <math.h>

/**
* This structure contains the information we need for each player.
*/
typedef struct players_struct
{
	char name[20]; 	/**< Player name */
	int table_number; 	/**< Table's number for this player */
}players;

void checkMainParameters(int argc, char **argv);
void *connection_handler(void *);
void bookInventory(inventory* server_inventory, int const inv_length, inventory* client_inventory, int const size_request, int const quota_per_player, int sock);

inventory* server_inventory; /**< Array that contains the servers inventory for every table. */

int num_of_player; /**< The number that needs a table to start. */
int players_counter = 0; /**< The number of all players. */
int tables = 0; /**< The number of tables. */
int inv_length=0; /**< The size of the servers inventory. */
int quota_per_player; /**< The quota of clients request. */
pthread_mutex_t mutex; /**< Mutex, semaphore, for critical region to read/write the tables inventory. */

pthread_mutex_t mutex_talk; /**< Mutex, semaphore, for critical region to talk the players. */
pthread_mutex_t mutex_table; /**< Mutex, semaphore, for critical region to create table. */
int	position=0; /**< The position in the table. */
int out_players = 0; /**< The number of clients has received START message. */

int socket_desc; /**< Socket descriptors returned from socket . */
int client_sockets[MAX_PLAYERS]; /**< This pointer is a array that hold all sockets clients. */
int connfd; /**< Socket descriptors returned from accept. */
inventory** static_server_inventory_ptr; /**< This pointer is an array to the starting inventory for each table. */
players c_player[MAX_PLAYERS]; /**< This is the array with the players */

int main(int argc , char *argv[])
{
	int clilen;
	struct sockaddr_in server , client;
	
	/* check for command line arguments */
	checkMainParameters(argc, argv);

	/* save the command line arguments */
	num_of_player = atoi( argv[2] ); 
	char* filename = argv[4];
	quota_per_player = atoi( argv[6] );

	int i=0;
	for(i=0; i<MAX_PLAYERS; i++) client_sockets[i]=-1; //initialize clients sockets

	/* read and save the servers inventory */
	server_inventory = readInventory(filename, &inv_length);
	inventory* static_server_inventory = (inventory *) malloc (inv_length*sizeof(inventory));
	memcpy(static_server_inventory, server_inventory, inv_length*sizeof(inventory)); //static_server_inventory = server_inventory

	static_server_inventory_ptr = &static_server_inventory; //ptr to static server inventory

	socket_desc = socket(AF_INET , SOCK_STREAM , 0); //create socket
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}

	/* Prepare the sockaddr_in structure */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( PORTNUM );

	/* binding */
	int yes=1;
 	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
 	{
  	  	fprintf(stderr, "setsockopt error!\n" );
    	exit(2);
	}
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("bind failed. Error");
		return 6;
	}

	listen(socket_desc , LISTENQ);

	/* Accept and incoming connection */
	clilen = sizeof(struct sockaddr_in);
	startingMessage(server_inventory, inv_length, quota_per_player, num_of_player);

	pthread_t thread_id;

	while( (connfd = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&clilen)) )
	{
		/* save the clients socket descriptor */
		
		while(client_sockets[position] != -1) position++;
		client_sockets[position] = connfd;

		if( pthread_create( &thread_id , NULL , connection_handler, (void*) &connfd) < 0)
		{
			perror("could not create thread");
			exit(3);
		}
	}
	if (connfd < 0)
	{
		perror("accept failed");
		exit(4);
	}

	return 0;
}

/**
* This function handle the thread that communicate with the client. First 
* read his request and decode it. Then we have a critical region because we
* have race condition with the server_iventory and with the table. For this
* reason we have a mutex semaphore to protect this code. Then the client received 
* a waiting message until to complete the table and start the game and the chat. 
* @param connfd The socket descriptor of client's connection
*/
void *connection_handler(void *connfd)
{
	/* Get the socket descriptor */
	int sock = *(int*)connfd;

	char *message, client_message[512];

	/* Receive a message from client */
	if( read(sock, client_message, sizeof(client_message)) == 0 ) 
	{
		fprintf(stderr, "Connection lost! \n" );
		int temp;
		pthread_exit(&temp); /* exit thread */
	}

	/* parsing client's name */
	char current_name[25];
	int i=0;
	while(client_message[i] != '\n')
	{
		c_player[position].name[i] = client_message[i];
		i++;
	}
	c_player[position].name[i] = '\0';

	printf("\nMessage from client:\n%s\n", client_message);
	strcpy(current_name, c_player[players_counter].name);

	/* calculate size of client's request */
	int size_request = calculateInventorySize(client_message);

	inventory* client_inventory = (inventory *) malloc ( size_request * sizeof(inventory) );

	/* this function explanation in the server what you read */
	decodeClientRequest(client_message,client_inventory);

	/*
	*	Critical region to check server's inventory
	*/
	pthread_mutex_lock(&mutex);
	
	/* check if we need a new table */
	
	if (floor(players_counter/num_of_player) == tables)
	{
		pthread_mutex_lock(&mutex_table); //lock semaphore. unlock when all clients at previous table get START message
		printf("\n+--------- NEW TABLE ---------+\n");

		bookInventory(server_inventory, inv_length , client_inventory, size_request, quota_per_player, sock);

		tables++;
		players_counter++;
		printf("\n%d players in table %d\n",players_counter,tables );
		/* the inventory has change */
		printf("New inventory for %d table\n",tables);
		printInventoryArray(server_inventory, inv_length);

		/* send OK message and increase the counter */
		message = "OK";
		write(sock , message , strlen(message)+1);
	}
	else
	{
		bookInventory(server_inventory, inv_length , client_inventory, size_request, quota_per_player, sock);
		
		players_counter++;
		printf("\n%d players in table %d\n",players_counter,tables );
		/* the inventory has change */
		printf("New inventory for table %d\n",tables);
		printInventoryArray(server_inventory, inv_length);

		/* send OK message and increase the counter */
		message = "OK";
		write(sock , message , strlen(message)+1);
	}

	/* set the player's table */
	c_player[players_counter-1].table_number = tables;
	
	int current_table;
	current_table = tables;
	
	pthread_mutex_unlock(&mutex);
	/*
	*	End of critical region
	*/

	/* wait until the table is full and send a message to wait every 5 seconds */
	sleep(5);

	while( players_counter%num_of_player != 0 )
	{
		message = "Please wait";
		write (sock, message, strlen(message)+1);
		sleep(5);
	}

	/* initialize the inventory for the new table */
	memcpy(server_inventory, *static_server_inventory_ptr, inv_length*sizeof(inventory));
	/* send start message */
	message = "START";
	write(sock , message , strlen(message)+1);
	out_players++;
	if (out_players == num_of_player*tables)
		pthread_mutex_unlock(&mutex_table); //unlock semaphore to create new table


	/* this loop read the message from the client and send to the others */
	while (TRUE)
	{
		if( read(sock, client_message, sizeof(client_message)) == 0 ) // read client's message
		{
			/*
			*	if client disconnect then we 
			* 	remove his socket descriptor 
			*	and  terminate the thread 
			*/
			int temp;
			pthread_mutex_lock(&mutex_talk);
			for (i = 0; i < players_counter; i++)
			{
				char send[50];
				if ( (sock == client_sockets[i]) )
				{
					printf("Client %s has left from the table (%d)\n", current_name, current_table);
				}
				
				if ( (current_table == c_player[i].table_number)  && (client_sockets[i] != -1) )
				{			
					strcpy(send, "Message from server: ");
					strcat(send, current_name);
					strcat(send, " has left from the table");
					write(client_sockets[i] , send , strlen(send)+1);
				}
			}
			pthread_mutex_unlock(&mutex_talk);
			for (i = 0; i < players_counter; i++)
			{
				if ( (sock == client_sockets[i]) )
				{
					client_sockets[i] = -1;
					break;
				}
			}
			free(client_inventory);
			pthread_exit(&temp); /* exit thread */
		}

		int i;

		/**
		 * Critical Region for client_sockets shared variable
		 */
		pthread_mutex_lock(&mutex_talk);
		for (i = 0; i < players_counter; i++)
		{
			if ( (sock == client_sockets[i]) )
			{
				printf("\n - %s from table %d said: %s \n",c_player[i].name, c_player[i].table_number, client_message);
				
			}
			
			if ( (current_table == c_player[i].table_number)  && (client_sockets[i] != -1) )
			{			
				char send[50];
				strcpy(send, current_name);
				strcat(send, ": ");
				strcat(send, client_message);
				write(client_sockets[i] , send , strlen(send)+1);
			}
		}
		pthread_mutex_unlock(&mutex_talk);
		/**
		 * End of critical region
		 */
	}
} 

/**
* This function check the main arguments 
* @param argc The number of arguments
* @param argv The array with the parameters 
*/
void checkMainParameters(int argc, char **argv)
{
	/* check for main parameters */
	if ( argc!=7  || strcmp(argv[1], "-p") || strcmp(argv[3], "-i") || strcmp(argv[5], "-q"))
	{
		fprintf(stderr, "Error: %s -p <num_of_player> -i <game_inventory> -q <quota_per_player> \n", argv[0]);
		exit(1);
	}
}

/**
* This function check the availability of the current table's inventory. If client's 
* request is not more than the maximum quota per player and the server inventory is 
* available then removed. If is not we remove his socket descriptor and terminate
* the thread.
* @param server_inventory The inventory of the current table
* @param inv_length The size of the table's inventory
* @param client_inventory The client's inventory
* @param size_request The size of the client's inventory
* @param quota_per_player The maximum quota of inventory per player
* @param sock The socket descriptor of the client
*/
void bookInventory(inventory* server_inventory, int const inv_length, inventory* client_inventory, int const size_request, int const quota_per_player, int sock)
{
	if ( invcmp(server_inventory, inv_length , client_inventory, size_request, quota_per_player) == 0 ) // if inventory is ok
	{
		int i;
		for (i = 0; i<size_request; i++)
		{
			int pos;
			pos=findInvName(server_inventory, inv_length, client_inventory[i].name);
			server_inventory[pos].quantity -= client_inventory[i].quantity;
		}
	}
	else // i can't give his what he ask
	{
		char* send = "Invalid inventory";
		printf("Player %d: %s \n", getpid(), send );
		write (connfd, send, strlen(send)+1);
		pthread_mutex_unlock(&mutex);
		int temp;
		int i;
		for (i = 0; i < players_counter; i++)
		{
			if ( (sock == client_sockets[i]) )
			{
				client_sockets[i] = -1;
				break;
			}
		}
		pthread_exit(&temp); /* exit thread */
	}

}
