/**
* @file sigHan.h
* @brief This file contains the function for the signal handlers 
*/

#ifndef __SIGHAN__
#define __SIGHAN__

#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include "consts.h"

extern int socket_desc; /**< Socket descriptors returned from socket . */
extern int connfd; /**< Socket descriptors returned from accept. */

/**
The use of this functions avoids the generation of
"zombie" processes.
*/
void sig_chld( int signo )
{
	pid_t pid;
	int stat;
	while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 ) 
	{
		printf( "Child %d terminated. Signo %d \n", pid, signo );
	}
}

/**
* The use of this function is to close open sockets when we terminate the program
*/
void sig_close(int signo)
{
	close(socket_desc);
	close(connfd);
	printf("Signo: %d\nBYE\n", signo);
	exit(0);
}

#endif