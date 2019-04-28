
all: libhash.a  test integer-count

libhash.a:  hash.c
	gcc -g -Wall -c hash.c
	ar -cvq libhash.a hash.o
	ranlib libhash.a

integer-count: integer-count.c
	gcc -g -Wall -o integer-count integer-count.c -L. -lhash -lpthread

test: test.c
	gcc -g -Wall -o test test.c -L. -lhash -lpthread

clean: 
	rm -fr *.o *.a *~ a.out integer-count x  test hash.o libhash.a
