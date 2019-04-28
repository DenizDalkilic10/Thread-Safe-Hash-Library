#include <stdio.h>
#include <pthread.h>
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "hash.h"
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <math.h>

HashTable *hash_init(int N, int K)
{
	int bucketValidity = 1, lockValidity = 1;

	if (N > MAX_N || N < MIN_N)
	{
		printf("Number of Buckets: %d is not valid \n", N);
		bucketValidity = 0;
	}
	if (K > MAX_N / MIN_M || K <= MIN_N / MAX_M)
	{
		printf("Number of Locks: %d is not valid \n", K);
		lockValidity = 0;
	}

	if (!bucketValidity || !lockValidity)
	{
		return NULL;
	}

	HashTable *hp;
	if ((hp = (HashTable *)malloc(sizeof(HashTable))) == NULL)
	{
		return NULL;
	}

	hp->N = N;	 //number of buckets
	hp->K = K;	 //number of locks
	hp->M = N / K; //size of a region
	hp->table = (Cell **)malloc(sizeof(Cell) * N);
	for (int i = 0; i < hp->N; i++)
	{
		hp->table[i] = NULL;
	}

	hp->lockVariables = malloc(sizeof(pthread_mutex_t) * (K + 1));
	for (int i = 0; i < K + 1; i++)
	{
		pthread_mutex_init(&hp->lockVariables[i], NULL); //initializing the necessary locks
	}
	return hp;
}

int my_hash(int value, int buckets)
{
	if (value <= 0)
	{
		return -1;
	}

	value = value % buckets;
	return value;
}

int hash_insert(HashTable *hp, int k, void *v)
{
	int j_bucket = my_hash(k, hp->N);

	if (j_bucket == -1)
	{
		return -1;
	}

	int lock = j_bucket / hp->M;
	pthread_mutex_lock(&(hp->lockVariables[lock]));
	//Critical Section
	Cell *head = hp->table[j_bucket];
	if (head == NULL)
	{
		hp->table[j_bucket] = (Cell *)malloc(sizeof(Cell));
		hp->table[j_bucket]->key = k;
		hp->table[j_bucket]->value = v;
		hp->table[j_bucket]->next = NULL;
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return 0;
	}

	while (head->next != NULL)
	{
		if (head->key < k && head->next->key > k)
		{
			Cell *add = (Cell *)malloc(sizeof(Cell));
			add->key = k;
			add->value = v;
			add->next = head->next;
			head->next = add;
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return 0;
		}

		if (head->key == k)
		{
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return -1;
		}
		head = head->next;
	}

	if (head->key == k)
	{
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return -1;
	}

	head->next = (Cell *)malloc(sizeof(Cell));
	head->next->key = k;
	head->next->value = v;
	head->next->next = NULL;
	pthread_mutex_unlock(&(hp->lockVariables[lock]));
	return 0;
}

int hash_delete(HashTable *hp, int k)
{
	int j_bucket = my_hash(k, hp->N);

	if (j_bucket == -1)
	{
		return -1;
	}

	int lock = j_bucket / hp->M;
	pthread_mutex_lock(&(hp->lockVariables[lock]));

	//Critical Section
	Cell *head = hp->table[j_bucket];
	if (head == NULL)
	{
		return -1;
	}

	if (head->next == NULL) //if head is the only element
	{
		if (head->key != k)
		{
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return -1;
		}

		free(hp->table[j_bucket]);
		hp->table[j_bucket] = NULL;
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return 0;
	}

	if ((hp->table[j_bucket]->key) == k)
	{
		head = hp->table[j_bucket];
		hp->table[j_bucket] = hp->table[j_bucket]->next;
		free(head);
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return 0;
	}

	while (head->next != NULL)
	{
		if ((head->next->key) == k)
		{
			Cell *temp = head->next;
			head->next = head->next->next;
			free(temp);
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return 0;
		}
		head = head->next;
	}

	pthread_mutex_unlock(&(hp->lockVariables[lock]));
	return -1;
}

int hash_update(HashTable *hp, int k, void *v)
{
	int j_bucket = my_hash(k, hp->N);

	if (j_bucket != -1)
	{
		return -1;
	}

	int lock = j_bucket / hp->M;
	pthread_mutex_lock(&(hp->lockVariables[lock]));
	//-- Critical Section Start --
	Cell *head = hp->table[j_bucket];
	if (head == NULL)
	{
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return -1;
	}

	Cell *curr = head;
	while (curr != NULL)
	{
		if (curr->key == k)
		{
			curr->value = v;
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return 0;
		}
		curr = curr->next;
	}

	pthread_mutex_unlock(&(hp->lockVariables[lock]));
	return -1;
}

int hash_get(HashTable *hp, int k, void **vp)
{
	int j_bucket = my_hash(k, hp->N);

	if (j_bucket == -1)
	{
		return -1;
	}

	int lock = j_bucket / hp->M;
	pthread_mutex_lock(&(hp->lockVariables[lock]));
	// -- Critical Section Start --
	Cell *head = hp->table[j_bucket];

	if (head == NULL)
	{
		pthread_mutex_unlock(&(hp->lockVariables[lock]));
		return -1;
	}

	Cell *curr = head;
	while (curr != NULL)
	{
		if (curr->key == k)
		{
			*vp = (curr->value);
			pthread_mutex_unlock(&(hp->lockVariables[lock]));
			return 0;
		}
		curr = curr->next;
	}

	pthread_mutex_unlock(&(hp->lockVariables[lock]));
	return -1;
}

int hash_destroy(HashTable *hp)
{
	pthread_mutex_lock(&(hp->lockVariables[hp->K]));
	Cell *current;
	for (int i = 0; i < hp->N; i++)
	{
		current = hp->table[i];
		Cell *prev = current;
		while (current != NULL)
		{
			current = current->next;
			free(prev->value);
			free(prev);
			prev = current;
		}
	}
	pthread_mutex_unlock(&(hp->lockVariables[hp->K]));

	free(hp->table);
	free(hp->lockVariables);
	free(hp);
	return (0);
}
