#include "consts.h" /* for user-defined constants */
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h> /* basic socket definitions */
#include <sys/types.h> /* basic system data types */
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h> /* for hostent struct */
#include "inventory.h"
#include "parsing.h"
#include <signal.h>

int sockfd;

void sig_chld( int signo )
{
	pid_t pid;
	int stat;
	while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 ) 
	{
		printf( "Child %d terminated. Signo %d \n", pid, signo );
	}
	close(sockfd);
	exit(0);
}

int main( int argc, char **argv )
{
	signal( SIGCHLD, sig_chld ); /* Avoid "zombie" process generation. */
	
	struct sockaddr_in servaddr; /* Struct for the server socket address. */
	struct hostent *server;

	char name[20];

	if ( argc!=6 || strcmp(argv[1],"-n") || strcmp(argv[3],"-i") )
	{
		fprintf(stderr, "Error: %s -n <name> -i <inventory> <server_host> \n", argv[0]);
		exit(1);
	}

	inventory* inv;
	strcpy(name, argv[2]);
	int inv_size;

	/* read content from client inventory file */
	inv = readInventory(argv[4], &inv_size);

	char toString[512] = "\0";
	char tempString[512] = "\0";

	printf("+----------------------------------------------------------------------------+\n");
	printf("|   _____          __  __ ______    _____ _      _____ ______ _   _ _______  |\n");
	printf("|  / ____|   /\\   |  \\/  |  ____|  / ____| |    |_   _|  ____| \\ | |__   __| |\n");
 	printf("| | |  __   /  \\  | \\  / | |__    | |    | |      | | | |__  |  \\| |  | |    |\n");
 	printf("| | | |_ | / /\\ \\ | |\\/| |  __|   | |    | |      | | |  __| | . ` |  | |    |\n");
 	printf("| | |__| |/ ____ \\| |  | | |____  | |____| |____ _| |_| |____| |\\  |  | |    |\n");
  	printf("|  \\_____/_/    \\_\\_|  |_|______|  \\_____|______|_____|______|_| \\_|  |_|    |\n");
  	printf("+----------------------------------------------------------------------------+\n");
                                                                           

	/* Create the client's endpoint. */
	sockfd = socket( AF_INET, SOCK_STREAM, 0 ); 
	
	if (sockfd <0)
	{
		perror("Error: ");
		exit(1);
	}

	server = gethostbyname(argv[5]);
	
	if ( server == NULL )
	{
		perror("Error: ");
		exit(1);
	}

	/* Zero all fields of servaddr. */
	bzero( (char *) &servaddr, sizeof( servaddr ) ); 

	servaddr.sin_family = AF_INET;

	/* Define the name of this socket. */
	bcopy( (char *)server->h_addr,(char*) &servaddr.sin_addr.s_addr, server->h_length ) ; 
	
	servaddr.sin_port = htons( PORTNUM );
	
	/* Connect the client's and the server's endpoint. */
	if ( connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("Error:" );
		exit(1);
	}

	char serv_answer[256];

	/*use two function to convert all in one string*/
	addToSend (toString,name);
	inventoryArrayToString(inv, tempString, inv_size);
	addToSend (toString,tempString);

	/*send string to server*/
	write (sockfd , toString , strlen(toString)+1);
	
	/*print string where sended*/
	printf("%s \n", toString);

	bzero( (char *)serv_answer, sizeof(serv_answer) );
	
	/*read answer from server*/
	if (read (sockfd, serv_answer, sizeof(serv_answer)) == 0 ) 
	{
		fprintf(stderr, "Server close connection! \n" );
		exit(1);
	}
	
	/*print server's answer*/
	printf("Server's answer  : %s\n",serv_answer);
	if ( strcmp(serv_answer, "OK") != 0)
	{
		exit(1);
	}

	while( strcmp(serv_answer, "START") != 0 )
	{
		/*read answer from server*/
		if (read (sockfd, serv_answer, sizeof(serv_answer)) == 0 ) 
		{
			fprintf(stderr, "Server close connection! \n" );
			exit(1);
		}
		/*print server's answer*/
		printf("Server's answer  : %s\n",serv_answer);
	}

	/* 
	*	create two process to
	* 	read and write parallel
	*/
	if( fork()==0 ) //read process
	{
		while( TRUE )
		{
			fflush(stdout);
			fflush(stdin);
			/*read answer from server*/
			if (read (sockfd, serv_answer, sizeof(serv_answer)) == 0 ) 
			{
				fprintf(stderr, "Server close connection! \n" );
				close( sockfd );
				exit(1);
			}
			/*print server's answer*/
			printf(" - %s\n",serv_answer);
		}	
	}
	else // write process
	{
		while( TRUE )
		{
			fflush(stdout);
			fflush(stdin);
			char buf_send[50];
			fgets(buf_send,49,stdin);

			if(write (sockfd , buf_send , strlen(buf_send)+1) < 0)
			{
				fprintf(stderr, "Server close connection! \n" );
				close( sockfd );
				exit(1);
			}
		}
	}
	close( sockfd );

	return 0;

}/*end main*/