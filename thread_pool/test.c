#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "thread_pool.h"
 
void *func(void *arg)
{
    printf("thread %d\n", (int)arg);
    return NULL;
}
 
int main(int arg, char **argv)
{
    int i;

    if (tpool_init(5) != 0) {
        printf("tpool_create failed\n");
        exit(1);
    }
    
    for (i = 0; i < 100; ++i) {
        tpool_add_work(func, (void*)i);
    }

    sleep(2);

    printf("Destroy the thread pool\n");
    tpool_destroy();

    return 0;
}
