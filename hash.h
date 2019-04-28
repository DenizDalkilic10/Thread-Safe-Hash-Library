#ifndef HASH_H
#define HASH_H

#include <pthread.h>

#define MIN_N 100
#define MAX_N 1000
#define MIN_M 10
#define MAX_M 1000

typedef struct Cell //right no key-value pair is corresponding to integer-count change it later for obtaining a general purpose hashtable
{
	int key;
	void *value;
	struct Cell *next; // to access the next node... for linked list structure
} Cell;

typedef struct hash_table
{
	int N;							//total number of buckets
	int M;							//number of buckets per region
	int K;							//number of regions
	struct Cell **table;			//hashtable itself
	pthread_mutex_t *lockVariables; //lock variables
} hash_table;

typedef struct hash_table HashTable;

HashTable *hash_init(int N, int K);
int hash_insert(HashTable *hp, int k, void *v);
int hash_delete(HashTable *hp, int k);
int hash_update(HashTable *hp, int, void *v);
int hash_get(HashTable *hp, int k, void **vp);
int hash_destroy(HashTable *hp);
int my_hash(int value, int buckets);

#endif /* HASH_H */