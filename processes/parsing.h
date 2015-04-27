/**
* @file parsing.h
* This header file contains function for parsing text 
*/

#ifndef __PARSING__
#define __PARSING__

#include <string.h>
#include "inventory.h"
#include <stdlib.h>

/**
* This method push back to the variable msg 
* the second string toAdd at the format:
* msg\ntoAdd
* @param msg The variable that we will add to
* @param toAdd The variable that we will add from
*/
void addToSend(char* msg, char* toAdd)
{
	msg = strcat(msg, "\n");
	msg = strcat(msg, toAdd); 
}

/**
* This method convert a inventory struct to string
* at format inv.name\tinv.quantity
* @param inv The inventory to convert
* @param toString The string that we will return
*/
void inventoryToString(inventory inv, char* toString)
{
	strcpy(toString, inv.name);
	strcat(toString, "\t");
	char buffer[10];
	sprintf(buffer, "%d", inv.quantity);
	strcat(toString,  buffer);
	strcat(toString, "\n");
}


/**
* This method convert a inventory array to string
* @param inv The inventory array
* @param toString The string that we will return
* @param arraySize The size of the inventory array
*/
void inventoryArrayToString(inventory* inv, char* toString, const int arraySize)
{
	int i=0;
	strcpy(toString, "\0");
	for( i=0; i<arraySize; i++)
	{
		char invStr[40];
		inventoryToString(inv[i], invStr);
		strcat(toString, invStr);
	}
}

/**
* This method take the client's request and decode the string inventory 
* to a structure inventory array
* @param request The client's request
* @param inv The array inventory that we will return
*/
char* decodeClientRequest(char* request, inventory* inv)
{
	char* name;
	name = strtok(request, "\t\n"); // read the name of the player and do nothing
	int i=0;
	char* temp = "";
	while(temp!=NULL)
	{
		temp = strtok(NULL,"\t\n"); // read the inventory name
		if (temp == NULL) break;
		strcpy(inv[i].name, temp);
		temp = strtok(NULL,"\t\n"); // read the inventory quantity
		inv[i].quantity = atoi(temp);
		i++;
	}	
	return name;
}


/**
* This method count the inventories of the request. The request must be at the format: \n
* name\n
* invetory1\n
* inventory2\n
* ....\n
* inventoryN\n
* @param request The string request
*/
int calculateInventorySize(char* request)
{
	int len = strlen(request);
	int i,lines=0; 
	for(i=0; i<len; i++)
	{
		if ( request[i] == '\t' ) lines++;
	}
	return lines;
}

/**
* This method print the starting message of the server
* @param the server's starting inventory
* @param inv_length the length of the server_inventory array
* @param quota_per_player the maximum sum of inventory can ask by a player
* @param num_of_player the players that can host in a table
*/
void startingMessage(inventory* server_inventory, const int inv_length, const int quota_per_player, const int num_of_player)
{
	printf("+-------------------------------------------------------------------------------------------------+\n");
	printf("|  _______  _______  __   __  _______     _______  _______  ______    __   __  _______  ______    |\n");
	printf("| |       ||   _   ||  |_|  ||       |   |       ||       ||    _ |  |  | |  ||       ||    _ |   |\n");
	printf("| |    ___||  |_|  ||       ||    ___|   |  _____||    ___||   | ||  |  |_|  ||    ___||   | ||   |\n");
	printf("| |   | __ |       ||       ||   |___    | |_____ |   |___ |   |_||_ |       ||   |___ |   |_||_  |\n");
	printf("| |   ||  ||       ||       ||    ___|   |_____  ||    ___||    __  ||       ||    ___||    __  | |\n");
	printf("| |   |_| ||   _   || ||_|| ||   |___     _____| ||   |___ |   |  | | |     | |   |___ |   |  | | |\n");
	printf("| |_______||__| |__||_|   |_||_______|   |_______||_______||___|  |_|  |___|  |_______||___|  |_| |\n");
	printf("|                                                                                                 |\n");
	printf("+-------------------------------------------------------------------------------------------------+\n");
	
	printf("\n\nFor exit press Ctr-C \n\n");
	printf("Starting inventory: \n");
	printInventoryArray(server_inventory, inv_length);
	printf("Quota per player: %d \n", quota_per_player);
	printf("Players per table: %d \n",num_of_player);	
	printf("\n");	
}

#endif
