/**
* @mainpage Multiplayer Online Game
*	This project is a game server for a multiplayer online 
*	game with specific number of players. Every new player
*	that connect to the server choose his starting inventory. 
*	When the number of players complete then start the game.
*	After the players start to play they can chat online via 
*	the server.
*/

/**
* @file server.c
*	This is the server's code. 
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <errno.h> /* for the EINTR constant */
#include <sys/wait.h> /* for the waitpid() system call */
#include <sys/un.h> /* for Unix domain sockets */
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include "consts.h"
#include "sigHan.h"
#include "inventory.h"
#include "parsing.h"
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include "myshm.h"

void checkMainParameters(int argc, char **argv);
void initListenSocket(int* listenfd, struct sockaddr_in* servaddr);
void bookInventory(inventory** server_inventory, int const inv_length, inventory* client_inventory, int const size_request, int const quota_per_player);

int connfd; /**< Socket descriptors */
int listenfd; /**< Socket descriptors */
char buffer[512];
int** client_sockets; 
int num_of_player;
char** shm_buffer;
inventory** s_shm;
int* players_counter_shm;
pid_t main_pid;
int shmid_array[SHMID_SIZE];

int main( int argc, char **argv )
{
	sem_t* mutex_server_inv; /* Semaphore for critical region that r/w at server's inventory */
	sem_t* mutex_tables; /* Semaphore for start game and enable to create another table */
	sem_t* mutex_buffer;
	main_pid = getpid();

	char send[512];

	inventory* server_inventory;

	checkMainParameters(argc, argv);

	num_of_player = atoi( argv[2] );
	char* filename = argv[4];
	int quota_per_player = atoi( argv[6] );

	int inv_length=0;
	server_inventory = readInventory(filename, &inv_length);
	
	socklen_t clilen;

	struct sockaddr_in cliaddr, servaddr; /* Structs for the client and server socket addresses. */

	signal( SIGINT, sig_close );
	signal( SIGCHLD, sig_chld ); /* Avoid "zombie" process generation. */
	
	initListenSocket(&listenfd, &servaddr);

 	/*
 	*	create shared memory here
 	*/
 	players_counter_shm = newIntSharedMemory(0, SMKEY_PLAYERS);
 	s_shm = newArrayInventoryArraySharedMemory(server_inventory, inv_length, SMKEY_SERVINV);
 	/*
 	*	end of initialize shared memory 
 	*/	

 	mutex_server_inv = sem_open(SNAME_SERVER_INV, O_CREAT, 0644, 1); /* create and itiliaze semaphore */ 	

 	listen( listenfd, LISTENQ ); /* Create request queue. */

 	startingMessage(server_inventory, inv_length, quota_per_player, num_of_player);
	
	mutex_tables = sem_open(SNAME_TABLE, O_CREAT, 0644, 1);	/* open semaphore mutex_tables */
	mutex_buffer = sem_open(SNAME_BUF, O_CREAT, 0644, 1);

	for ( ; ; )
	{
		sem_wait(mutex_tables);

		clilen = sizeof( cliaddr );
		/* Copy next request from the queue to connfd and remove it from the queue. */
		connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );

 		if ( fork() == CHILD_PROCESS ) //new table
 		{
 			int table_pid = getpid();
 			signal(SIGUSR1, sendMessagesToAll);

 			client_sockets = (int **) newIntArraySharedMemory(num_of_player, 0);

 			shm_buffer = (char **)newIntArraySharedMemory(512, SMKEY_BUFFER); 
 			memcpy(*s_shm, server_inventory, inv_length*sizeof(server_inventory));

 			printf("\n+------ NEW TABLE: %d ------+\n", getpid());

 			int* table_players = newIntSharedMemory(0,table_pid);
 			
 			for( ; *table_players<num_of_player; )
 			{
 				(*client_sockets)[*table_players] = connfd;
 				if ( fork()==0 ) //new player
 				{
 					break;
 				}
 				else
 				{
			 		sem_t* mutex_check_num = sem_open(SNAME_CHN, O_CREAT, 0644, 0);	/* open semaphore mutex_tables */
			 		sem_wait(mutex_check_num);

			 		if ( *table_players == num_of_player )
			 		{
			 			mutex_tables = sem_open(SNAME_TABLE, O_CREAT, 0644, 1);	/* open semaphore mutex_tables */
						sem_post(mutex_tables);
			 			int temp_counter = num_of_player;			
						while( (temp_counter--) > 0 ) wait(NULL);
			 			printf("Table %d closed\n", getpid());
			 		}
 				}
 				clilen = sizeof( cliaddr );
				/* Copy next request from the queue to connfd and remove it from the queue. */
				connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen );

				if ( connfd < 0 )
				{
					if ( errno == EINTR ) /* Something interrupted us. */
						continue; /* Back to for()... */
					else
					{
		 				fprintf( stderr, "Accept Error\n" );
		 				exit( 0 );
		 			}
		 		}
 			}
 			close( listenfd ); /* Close listening socket. */	

 			bzero( (char *)buffer, sizeof(buffer) );
							
			if( read(connfd, buffer, sizeof(buffer)-1) == 0 ) // recv message from client
			{
				fprintf(stderr, "Connection lost! \n" );
				exit(1);
			}
	
			printf("\nMessage from client (%d): %s\n", getpid(), buffer);

			/* calculate size of client's request */
			int size_request = calculateInventorySize(buffer);
			
			inventory* client_inventory = (inventory *) malloc ( size_request * sizeof(inventory) );
			char* name;
			
			/* this function explanation in the server what you read */
			name = decodeClientRequest(buffer,client_inventory);
			
			sem_t* mutex_server_inv = sem_open(SNAME_SERVER_INV, O_CREAT, 0644, 1); /* open semaphore mutex_server_inv */ 
			
			/*
			*	Critical region to check server's inventory
			*/	
			
			sem_wait(mutex_server_inv);
			bookInventory(s_shm, inv_length , client_inventory, size_request, quota_per_player);
			(*players_counter_shm)++;
			(*table_players)++;
			printf("%d players in %d table\n", *table_players, getppid());
			sem_post(mutex_server_inv);
			sem_t* mutex_check_num = sem_open(SNAME_CHN, O_CREAT, 0644, 0);	/* open semaphore mutex_tables */
	 		sem_post(mutex_check_num);
			/*
			*	End of critical region 
			*/

			printf("New inventory for %d table \n", getppid());
			printInventoryArrayPtr(s_shm, inv_length);	

			write (connfd, "OK", strlen("OK")+1);
			strcpy(send, "please wait");

			sleep(5);
			
			while( *table_players != num_of_player )
			{
				write(connfd, send, strlen(send)+1);
				sleep(5);
			}
		
			strcpy(send, "START");
			write(connfd, send, strlen(send)+1);

			while( TRUE )
			{
				char read_buf[50];
				if( read(connfd, read_buf, sizeof(read_buf)-1) == 0 ) // recv message from client
				{
					printf("Client %s (%d) left from the table \n",name, getpid() );
					int i=0;
					while( (*client_sockets)[i] != connfd ) i++;
					(*client_sockets)[i] =-1;
					mutex_buffer = sem_open(SNAME_BUF, O_CREAT, 0644, 1);
					sem_wait(mutex_buffer);
					strcpy(*shm_buffer, "Message from server: ");
					strcat(*shm_buffer, name);
					strcat(*shm_buffer, " has left from the table");
					kill(table_pid, SIGUSR1);
					(*table_players)--;
					exit(1);
				}
				mutex_buffer = sem_open(SNAME_BUF, O_CREAT, 0644, 1);
				sem_wait(mutex_buffer);
				strcpy(*shm_buffer, name);
				strcat(*shm_buffer, ": ");
				strcat(*shm_buffer, read_buf);
				kill(table_pid, SIGUSR1);
				printf("\n - %s (%d) from %d said: %s \n",name, getpid(), table_pid, read_buf);
			}

			exit( 0 ); /* Terminate child process. */
 		}
 		else // main process
 		{

 		}

 	}/*end for (; ;)*/

	return 0;

}/*end of main*/


/**
* This function check the main arguments 
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
* This function initialise the server's socket
*/
void initListenSocket(int* listenfd, struct sockaddr_in* servaddr)
{
	
	listenfd[0] = socket( AF_INET, SOCK_STREAM, 0 ); /* Create the server's endpoint */
	
	if (listenfd[0] <0)
	{
		fprintf(stderr, "Error opening socket\n" );
		exit(1);
	}

	bzero( servaddr, sizeof( servaddr[0] ) ); /* Zero all fields of servaddr. */
	
	servaddr[0].sin_family = AF_INET; 
	servaddr[0].sin_port = htons( PORTNUM );
	servaddr[0].sin_addr.s_addr = INADDR_ANY;

	/* Create the file for the socket and register it as a socket. */
	int yes=1;
 	
 	if (setsockopt(listenfd[0], SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
 	{
  	  	fprintf(stderr, "setsockopt error!\n" );
    	exit(1);
	}

 	if ( bind( listenfd[0], ( struct sockaddr* ) servaddr, sizeof( servaddr[0] ) ) < 0 )
 	{
 		fprintf(stderr, "Error on binding\n" );
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
*/
void bookInventory(inventory** server_inventory, int const inv_length, inventory* client_inventory, int const size_request, int const quota_per_player)
{
	if ( invcmp(server_inventory, inv_length , client_inventory, size_request, quota_per_player) == 0 ) // he is ok!!
	{
		int i;
		for (i = 0; i<size_request; i++)
		{
			int pos;
			pos=findInvName(server_inventory, inv_length, client_inventory[i].name);
			server_inventory[pos]->quantity -= client_inventory[i].quantity;
		}
	}
	else // i can't give his what he ask
	{
		char* send = "Invalid inventory";
		printf("Player %d: %s \n", getpid(), send );
		write (connfd, send, strlen(send)+1);
		sem_t* mutex_server_inv = sem_open(SNAME_SERVER_INV, O_CREAT, 0644, 1); /**< open semaphore mutex_server_inv */ 
		sem_post(mutex_server_inv);
		sem_t* mutex_check_num = sem_open(SNAME_CHN, O_CREAT, 0644, 0);	/**< open semaphore mutex_tables */
 		sem_post(mutex_check_num);
		exit(1);
	}
}
