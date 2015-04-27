/**
* @file inventory.h
* @brief Inventory structure
* This file contains the inventory structure and method's to use it.
*/

#ifndef __INVENTORY__
#define __INVENTORY__

#include <stdio.h>
#include <string.h>
#include "consts.h"

/**
* This is a structure for inventory and contains the name of the inventory
* and the quantity 
*/
typedef struct inventory
{
	char name[20]; 	/**< inventory name */
	int quantity; 	/**< inventory quantity */
}inventory;

/**
* This function read a inventory from a file and return a inventory array and his size at the argument size
* @param fileName the name of the file
* @param len the length of the inventory array
* @return the inventory array
*/
inventory* readInventory(char* fileName, int* len)
{
	FILE* fp;
	inventory* toReturn;

	if ( (fp = fopen(fileName, "r")) == NULL ) {
		fprintf(stderr, "Can not open file \n");
		exit(1);
	}

	/* count lines */
	int lines=0;
	lines++;
	char ch;
	while ((ch = fgetc(fp)) != EOF)
	{
		if (ch == '\n')
			lines++;
	}
	/* end of count lines */

	toReturn = (inventory *) malloc(lines * sizeof(inventory));
	rewind(fp);

	/* read file */
	char line[50];
	int i=0;
	while (  fgets(line, 50, fp) != NULL )
	{
		strcpy( toReturn[i].name, strtok(line, "\t\n") );
		toReturn[i].quantity = atoi(strtok(NULL, "\t\n"));
		i++;
	}
	*len = i; /* size of inventory[] to return */

	fclose(fp);
	return toReturn;
}

/**
* @brief This function print a inventory structure
* @param inv The inventory to print
*/
void printInventory(const inventory inv)
{
	printf("%s\t%d \n", inv.name, inv.quantity);
}


/**
* @brief This function print a inventory array
* @param inv The inventory array
* @param len The size of the array
*/
void printInventoryArray(const inventory inv[], const int len)
{
	int i;
	for(i=0; i<len; i++)
		printInventory(inv[i]);
}

/**
 * @brief This function print a pointer to inventory array
 * 
 * @param inv The pointer to the array
 * @param len The size of array
 */
void printInventoryArrayPtr(inventory **inv, const int len)
{
	int i;
	for(i=0; i<len; i++)
	{
		printf("%s\t%d\n", inv[i]->name, inv[i]->quantity);
	}
}

/**
* This function find the position of name at the inventory table
* @param inv The inventory array
* @param size The size of the array
* @param name[] The name to search for
*/
int findInvName(inventory **inv, int size, char name[])
{
	int i;
	for (i = 0; i < size; i++)
	{
		if ( strcmp(inv[i]->name, name) == 0) 
		{
			return i;
		}
	}
	return NOT_FOUND;
}

/**
* This function check if the resource name exist in the inventory
* @param inv the inventory array we will search in
* @param size the size of the inventory array
* @param name the name of the inventory we are looking for
*/
int checkInvName(inventory **inv, int size ,char name[])
{
	if ( findInvName(inv, size, name) == NOT_FOUND )
	{
		return FALSE;
	}
	return TRUE;
}

/**
* This function compare two inventories inv1 and inv2
* @param inv1 the first inventory to compare
* @param len1 the length of the first inventory
* @param inv2 the second inventory to compare
* @param len2 the length of the second inventory
* @return this function return 0 if inv2 is equal with inv1 or subset 
*			and the sum of inventories of inv2 is les than quota_per_player,
*			else return -1
*/
int invcmp(inventory** inv1, const int len1, inventory* inv2, const int len2, const int quota_per_player )
{
	if (len2 > len1) return -1;
	int i=0;
	int sum=0;
	
	for (i = 0; i<len2; i++)
	{
		int pos=-1;
		if ( (pos=findInvName(inv1, len1, inv2[i].name)) == NOT_FOUND )
		{
			return -1;
		}
		else
		{
			if ( inv1[pos]->quantity < inv2[i].quantity )
			{
				return -1;
			}
			else
			{
				sum += inv2[i].quantity;
				if ( sum > quota_per_player )
				{
					return -1;
				}
			}
		}
	}
	return 0;
}


#endif