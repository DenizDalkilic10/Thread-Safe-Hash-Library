#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "hash.h"
#include <sys/time.h>

HashTable *ht; // space allocated inside library
pthread_mutex_t *lock_variables;

void *hashOperation(void *no_of_operations)
{
	int seed = 0;
	srand(seed);

	for (int i = 0; i < *(int *)no_of_operations; i++)
	{
		int key = rand() % 1000 + 1;
		void *vp;

		int j_bucket = my_hash(key, ht->N); //finds the target bucket for key
		int lock = j_bucket / ht->M;		//finds the appropriate lock

		pthread_mutex_lock(&lock_variables[lock]); //acquire lock

		int present = hash_get(ht, key, &vp);
		int newVal;
		if (present == 0)
		{
			int oldVal = *(int *)vp; //get the old val
			newVal = oldVal + 1;	 //increase it by 1
			*(int *)vp = newVal;	 //write the new val
			hash_update(ht, key, vp);
		}
		else
		{
			newVal = 1;				  //new value is 1
			vp = malloc(sizeof(int)); //allocate space
			*(int *)vp = newVal;	  // write new val
			hash_insert(ht, key, vp);
		}
		pthread_mutex_unlock(&lock_variables[lock]); //release lock
	}

	pthread_exit(NULL);
}

// test, W, T, N, K
int main(int argc, char **argv)
{
	int operation_count = atoi(argv[1]);
	int thread_count = atoi(argv[2]);
	int N = atoi(argv[3]);
	int K = atoi(argv[4]);
	int operation_per_thread = operation_count / thread_count;
	pthread_t threads[thread_count];

	printf("-- START! --\n");
	printf("-- W: %d --\n", operation_count);
	printf("-- T: %d --\n", thread_count);
	printf("-- N: %d --\n", N);
	printf("-- K: %d --\n", K);

	ht = hash_init(N, K);

	lock_variables = malloc(K * sizeof(pthread_mutex_t));
	for (int i = 0; i < K; i++)
	{
		pthread_mutex_init(&lock_variables[i], NULL); //initializing the necessary locks
	}

	//-- measurement start --
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);

	int i;
	for (i = 0; i < thread_count; i++)
	{
		pthread_create(&threads[i], NULL, hashOperation, &operation_per_thread);
	}

	for (i = 0; i < thread_count; i++)
	{
		pthread_join(threads[i], NULL);
	}

	gettimeofday(&tv2, NULL);
	//-- measurement end --

	printf("-- DONE! --\n");
	printf("Total time = %f seconds\n",
		   (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
			   (double)(tv2.tv_sec - tv1.tv_sec));

	// write_result(ht);
	hash_destroy(ht);
	free(lock_variables);
	return 0;
}