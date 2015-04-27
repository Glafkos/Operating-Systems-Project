/**
* @file consts.h
* This file contains the constants variables that we use in all the program (server & client).
*/

#ifndef __CONSTS__
#define __CONSTS__
#define PORTNUM 51717 /**< The port number that client and server communicate */
#define CHILD_PROCESS 0 /**< Constant for the process id of a child process */
#define SHMID_SIZE 100
#define NOT_FOUND -1 /**< This constant is for our searches, the value we return when we didn't find it */
#define FALSE 0 /**< Boolean false value */
#define TRUE !(FALSE) /**< Boolean true value */
#define LISTENQ 20 /**< Size of the request queue. */
#define SNAME_TABLE "/mutex_tables" /**< Define semaphore name for tables */
#define SNAME_SERVER_INV "/mutex_server_inv" /**< Define semaphore's name for server's inventory */
#define SNAME_CHN "/mutex_check_num" /**< Define semaphore's name for check number of players in the table */
#define SNAME_BUF "/mutex_buffer" /**< Define semaphore's name for buffer shared memory */
#define SMKEY_PLAYERS 6000 /**< Key for number of players shared memory */
#define SMKEY_SERVINV 5678 /**< Key for server's inventory shared memory */
#define SMKEY_BUFFER 9000 /**< Key for buffer that send the message in shared memory */
#endif