/**
* @file sigHan.h
* This file contains the function for the signal handlers 
*/

#ifndef __SIGHAN__
#define __SIGHAN__

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "consts.h"
#include "inventory.h"
#include <fcntl.h>

extern int listenfd; /**< Socket descriptors. */
extern int connfd; /**< Socket descriptors. */
extern sem_t* mutex_tables;
extern sem_t* mutex_server_inv;
extern char buffer[512];
extern int** client_sockets;
extern int num_of_player;
extern char** shm_buffer;
extern int* players_counter_shm;
extern inventory** s_shm;
extern int shmid_array[SHMID_SIZE];
extern pid_t main_pid;

/**
*	The use of this functions avoids the generation of
*	"zombie" processes.
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
	close(listenfd);
	close(connfd);
	/*
	* unlink all semaphores
	*/
	sem_unlink(SNAME_TABLE);
	sem_unlink(SNAME_SERVER_INV);
	sem_unlink(SNAME_CHN);
	sem_unlink(SNAME_BUF);
	
	/*
	* detach all shared memory
	*/
	shmdt(shm_buffer);
	shmdt(players_counter_shm);
	shmdt(s_shm);
	
	/*
	* destroy all shared memory segments
	*/
	int i=0;
	while(i<SHMID_SIZE)
	{
		if ( shmid_array[i]!=-1 )
		{
			shmctl(shmid_array[i], IPC_RMID, NULL);
		}
		i++;
	}
	if (getpid() == main_pid)
	{
		printf("Signo: %d\nBYE\n", signo);
	}
	exit(0);
}

/**
 * @details This function call it to send message to all players in the table
 * 
 */
void sendMessagesToAll( )
{
	signal(SIGUSR1, sendMessagesToAll);
	int i=0;
	for(i=0; i<num_of_player; i++)
	{
		if ( (*client_sockets)[i] != -1 )
		{
			write( (*client_sockets)[i], *shm_buffer, strlen(*shm_buffer)+1 );
		}
	}
	sem_t* mutex_buffer;
	mutex_buffer = sem_open(SNAME_BUF, O_CREAT, 0644, 1);
	sem_post(mutex_buffer);
}

#endif
