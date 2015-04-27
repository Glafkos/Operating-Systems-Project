/**
* @file consts.h
* @brief This header file contains the constants variables that we use in all the program (server & client).
*/

#ifndef __CONSTS__
#define __CONSTS__

#define PORTNUM 51717 /**< The port number that client and server communicate */
#define NOT_FOUND -1 /**< This constant is for our searches, the value we return when we didn't find it */
#define FALSE 0 /**< Boolean false value */
#define TRUE !(FALSE) /**< Boolean true value */
#define LISTENQ 20 /**< Size of the request server's queue. */
#define MAX_PLAYERS 100 /**< The maximun number of players that server can serve */

#endif