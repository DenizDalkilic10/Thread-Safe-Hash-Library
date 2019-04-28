1)
In the example code on GitHub, inside the test.c, the insert was written as:

hash_insert (ht1, i, (void *) 35000);

after we asked on piazza, we instead decided to use the void* to allocate space for an integer and pass the void* to the method like,

void *vp;
newVal = 1;			//new value is 1
vp = malloc(sizeof(int)); 	//allocate space
*(int *)vp = newVal;	  	//write new val
hash_insert(ht, key, vp);	//insert the new value

2)
Our hash table is thread safe thanks to the mutex locks we used. However, we again had to use mutex lock inside the test and integer-count files because we used "get" and "update" methods sequentially in our programs and we needed that operation to be atomic as well.
