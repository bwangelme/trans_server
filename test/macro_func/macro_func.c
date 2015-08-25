#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define PTHREAD_DETACH_CREATE(func, arg)\
{					\
	int 		err;		\
	pthread_t	tid;		\
	pthread_attr_t	attr;		\
					\
	err = pthread_attr_init(&attr);	\
	if(err != 0) {			\
		fprintf(stderr, "[pthread_detach_create]: %s\n", strerror(errno));\
		return -1;		\
	}				\
	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	\
	if(0 == err)			\
		err = pthread_create(&tid, &attr, (func), (arg));		\
	pthread_attr_destroy(&attr);	\
}


void *func(void *arg)
{
	int fd = *(int *)arg;
	printf("This is a detach thread[%d]\n", fd);
}

int main(int argc,char *argv[])
{
	int fd = 3;

	PTHREAD_DETACH_CREATE(func, &fd);
	sleep(3);

	return 0;
}
