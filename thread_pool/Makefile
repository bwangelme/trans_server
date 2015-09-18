lib:
	gcc -o libthread_pool.so -shared -fPIC thread_pool.c -lpthread

test: test.o thread_pool.c thread_pool.h
	gcc -o test test.o -L. -lthread_pool -lpthread
test.o: test.c

main: thread_pool.c thread_pool.h
	gcc -o bin thread_pool.c -lpthread
