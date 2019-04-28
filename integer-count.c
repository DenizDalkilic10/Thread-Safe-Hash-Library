#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "hash.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

typedef struct DataNode
{
	int key;
	void *value;
	struct DataNode *next;
} DataNode;

DataNode *MergeSortedLists(DataNode *part1, DataNode *part2)
{
	DataNode *result = NULL;

	if (part1 == NULL)
	{
		return (part2);
	}

	if (part2 == NULL)
	{
		return (part1);
	}

	if (part1->key <= part2->key)
	{
		result = part1;
		result->next = MergeSortedLists(part1->next, part2);
	}
	else
	{
		result = part2;
		result->next = MergeSortedLists(part1, part2->next);
	}
	return (result);
}

void SplitList(DataNode *head,
			   DataNode **firstPartPtr, DataNode **secondPartPtr)
{
	DataNode *end;
	DataNode *mid;
	mid = head;
	end = head->next;

	while (end != NULL)
	{
		end = end->next;
		if (end != NULL)
		{
			mid = mid->next;
			end = end->next;
		}
	}

	*firstPartPtr = head;
	*secondPartPtr = mid->next;
	mid->next = NULL;
}

void MergeSort(DataNode **headRef)
{
	DataNode *head = *headRef;
	DataNode *part1;
	DataNode *part2;

	if (head == NULL)
	{
		return;
	}

	if (head->next == NULL)
	{
		return;
	}

	SplitList(head, &part1, &part2);
	MergeSort(&part1);
	MergeSort(&part2);

	*headRef = MergeSortedLists(part1, part2);
}

HashTable *ht;
pthread_mutex_t *lock_variables;

void *readandUpdate(void *file)
{
	if (ht == NULL)
	{
		printf("Error with hash table. \n");
		exit(1);
	}

	int fd = 0;
	if ((fd = open((char *)file, O_RDONLY)) < 0)
	{
		printf("Error opening file. \n");
		exit(1);
	}

	// opens the input file, fp points to beginning of the file
	FILE *fp = fdopen(fd, "r");
	printf("File opened. \n");

	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	do
	{
		read = getline(&line, &len, fp); //for reading from input file

		//if the value is not positive
		if (read <= 0)
		{
			continue;
		}

		int key = atoi(line);
		void *vp;

		int j_bucket = my_hash(key, ht->N); //finds the target bucket for key
		int lock = j_bucket / ht->M;		//finds the appropriate lock

		pthread_mutex_lock(&lock_variables[lock]); //acquire lock
		int present = hash_get(ht, key, &vp);
		int newVal;
		if (present == 0)
		{
			int oldVal = *(int *)vp;  //get the old val
			newVal = oldVal + 1;	  //increase it by 1
			*(int *)vp = newVal;	  //write the new val
			hash_update(ht, key, vp); //update the hash table
		}
		else
		{
			newVal = 1;				  //new value is 1
			vp = malloc(sizeof(int)); //allocate space
			*(int *)vp = newVal;	  // write new val
			hash_insert(ht, key, vp); //insert in to hashtable
		}
		pthread_mutex_unlock(&lock_variables[lock]); //release lock
	} while (read != -1);

	free(line);
	fclose(fp);

	printf("Finished reading\n");
	pthread_exit(NULL);
}

int write_result(HashTable *hp, char* file_name)
{
	DataNode *head = malloc(sizeof(DataNode));
	head->next = NULL;
	DataNode *currentNode = head;

	struct Cell *current;
	for (int i = 0; i < hp->N; i++)
	{
		current = hp->table[i];
		while (current != NULL)
		{
			if (currentNode->next == NULL)
			{
				currentNode->next = malloc(sizeof(DataNode));
				currentNode = currentNode->next;
			}
			currentNode->key = current->key;
			currentNode->value = current->value;
			currentNode->next = NULL;
			current = current->next;
		}
	}
	currentNode = head->next;
	free(head);
	head = currentNode;

	//sort the linked list
	MergeSort(&head);

	//-- file write started --
	FILE *fp;
	fp = fopen(file_name, "w");

	if (fp < 0)
	{
		printf("Error opening output file");
	}

	currentNode = head;
	while (currentNode != NULL)
	{
		fprintf(fp, "%d: %d\n", currentNode->key, *(int *)currentNode->value);
		currentNode = currentNode->next;
	}
	fclose(fp);
	//-- file write finished --

	//-- delete start
	DataNode *prev;
	prev = head;
	currentNode = head;
	while (currentNode != NULL)
	{
		currentNode = currentNode->next;
		free(prev);
		prev = currentNode;
	}
	//-- delete end

	return 0;
}

//./integer-count 10 t_files/f_0.txt t_files/f_1.txt t_files/f_2.txt t_files/f_3.txt t_files/f_4.txt t_files/f_5.txt t_files/f_6.txt t_files/f_7.txt t_files/f_8.txt t_files/f_9.txt out.txt
int main(int argc, char **argv)
{
	printf("Multithreading Application...\n");

	int N = 1000;
	int K = 25;

	if (argv[1] == NULL)
	{
		printf("No Parameters \n");
		return -1;
	}

	int noOfThreads = atoi(argv[1]); //will be equal to the number of files
	pthread_t threads[noOfThreads];
	ht = hash_init(N, K);

	lock_variables = malloc(sizeof(pthread_mutex_t) * (K + 1));
	for (int i = 0; i < K + 1; i++)
	{
		pthread_mutex_init(&lock_variables[i], NULL); //initializing the necessary locks
	}

	int i;
	for (i = 0; i < noOfThreads; i++)
	{
		pthread_create(&threads[i], NULL, readandUpdate, argv[i + 2]);
	}

	for (i = 0; i < noOfThreads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	write_result(ht, argv[argc - 1]);
	hash_destroy(ht);
	free(lock_variables);
	printf("-- DONE! --\n");

	return 0;
}