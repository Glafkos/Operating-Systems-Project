/**
* @file inventory.h
* @brief This file contains the inventory structure and some functions for it.
*/

#ifndef __INVENTORY__
#define __INVENTORY__

#include <stdio.h>
#include <string.h>
#include "consts.h"

/**
* This is a structure for inventory and contains the name of the inventory
* and the quantity.
*/
typedef struct inventory
{
	char name[20]; 	/**< inventory name */
	int quantity; 	/**< inventory quantity */
}inventory;

/**
* This function read a inventory from a file and return the inventory array and 
* its size at len parameter of the function.
* @param fileName The name of the file
* @param len The length of the inventory array
* @return The inventory array
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
* This function print a inventory structure.
* @param inv The inventory for print
*/
void printInventory(const inventory inv)
{
	printf("%s\t%d \n", inv.name, inv.quantity);
}


/**
* This function print a inventory array.
* @param inv[] The inventory array for print
* @param len The length of the inventory array
*/
void printInventoryArray(const inventory inv[], const int len)
{
	int i;
	for(i=0; i<len; i++)
		printInventory(inv[i]);
}

/**
* This function find the position of name at the inventory table
* @param inv[] The inventory array
* @param size The size of inventory array
* @param name[] The name where that looking for its position
*/
int findInvName(inventory inv[], int size, char name[])
{
	int i;
	for (i = 0; i < size; i++)
	{
		if ( strcmp(inv[i].name, name) == 0) 
		{
			return i;
		}
	}
	return NOT_FOUND;
}

/**
* This function check if the resource name exist in the inventory
* @param inv[] The inventory array we will search in
* @param size The size of the inventory array
* @param name[] The name of the inventory we are looking for
*/
int checkInvName(inventory inv[], int size ,char name[])
{
	if ( findInvName(inv, size, name) == NOT_FOUND )
	{
		return FALSE;
	}
	return TRUE;
}

/**
* This function compare two inventories inv1 and inv2
* @param inv1 The first inventory to compare
* @param len1 The length of the first inventory
* @param inv2 The second inventory to compare
* @param len2 The length of the second inventory
* @return This function return 0 if inv2 is equal with inv1 or subset, else -1
*/
int invcmp(inventory* inv1, const int len1, inventory* inv2, const int len2, const int quota_per_player )
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
			if ( inv1[pos].quantity < inv2[i].quantity )
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