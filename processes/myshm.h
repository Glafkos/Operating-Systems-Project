/**
 * @file myshm.h
 * This header file contains functions to create and attach shared memory
 */

#include <sys/ipc.h>
#include <sys/shm.h>

extern int shmid_array[SHMID_SIZE]; /**< Array with shared memory ids to can destroy them */

/**
* This function create and attach a int shared memory
* @param value The value to initialize shared memory
* @param key The key for the shared memory
*/
int* newIntSharedMemory(int value, key_t key)
{
	int shmid_players; // return value from shmget()
 	int* players_counter_shm=NULL;
 	if ( ( shmid_players = shmget( key, sizeof(int), IPC_CREAT  | 0666 ) ) < 0)
 	{
 		perror("Shmget Faild (shmid_players) ");
 		exit(3);
 	}

 	int i=0;
 	while(shmid_array[i]==-1 && i<SHMID_SIZE) i++;
 	shmid_array[i] = shmid_players;

 	/* attach segment to shared memory */
 	 if ( (players_counter_shm = shmat(shmid_players, 0, 0)) == (int *) - 1)
 	 {
 	 	perror("Shmat Faild (players_counter_shm) ");
 	 	exit(4);
 	 }
 	*players_counter_shm = value;
 	return players_counter_shm;
}

/**
* This function create a new segment of shared memory type of inventory array and 
* initialize its value
* @param inv The inventory array to initialize shared memory
* @param size Number of array elements
* @param key The key for the shared memory
*/
inventory** newArrayInventoryArraySharedMemory(inventory inv[], const int size, key_t key)
{
 	int shmid; // return value from shmget()

 	inventory** s_shm = (inventory **) malloc( size*sizeof(inventory*) );
 	int i;
 	for(i=0; i<size; i++)
 	{
	 	if ( ( shmid = shmget( key+i, sizeof(inventory), IPC_CREAT  | 0666 ) ) < 0)
	 	{
	 		perror("Shmget Faild:");
	 		exit(1);
	 	}

	 	int j=0;
 		while(shmid_array[i]==-1 && j<SHMID_SIZE) j++;
 		shmid_array[j] = shmid;

	 	//attach segment to shared memory
	 	if ( (s_shm[i] = shmat(shmid, 0, 0)) == (inventory *) - 1)
	 	{
	 		perror("Shmat Faild: ");
	 		exit(2);
	 	}

	 	strcpy(s_shm[i]->name, inv[i].name);
	 	s_shm[i]->quantity = inv[i].quantity; 
 	}
 	return s_shm;
}

/**
* This function create a new segment of shared memory type of integer array and 
* initialize every element with -1
* @param size The size of the array
* @param key The key for the shared memory
*/
int ** newIntArraySharedMemory(const int size, key_t key)
{
 	int** int_array_ptr = (int **) malloc( size*sizeof(int*) );
 	int i;
 	int shmid;
 	for(i=0; i<size; i++)
 	{
	 	if ( ( shmid = shmget( key+i, sizeof(int), IPC_CREAT  | 0666 ) ) < 0)
	 	{
	 		perror("Shmget Faild:");
	 		exit(1);
	 	}

	 	int j=0;
 		while(shmid_array[i]==-1 && j<SHMID_SIZE) j++;
 		shmid_array[j] = shmid;

	 	//attach segment to shared memory
	 	if ( (int_array_ptr[i] = shmat(shmid, 0, 0)) == (int *) - 1)
	 	{
	 		perror("Shmat Faild: ");
	 		exit(2);
	 	}
	 	*int_array_ptr[i] = -1;
 	}
 	return int_array_ptr;
}