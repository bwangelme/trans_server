#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
 
#include "thread_pool.h"
 
static tpool_t *tpool = NULL;
 
/* 工作者线程函数, 从任务链表中取出任务并执行 */
static void* thread_routine(void *arg)
{
	tpool_work_t *work;
	int timeout = 0, ret;
	struct timespec tsp;
	struct timeval now;

	/* 如果线程池没有被销毁且没有任务要执行，则等待 */
	while(1) {
		pthread_mutex_lock(&tpool->queue_lock);
		tpool->idle++;
		while(NULL == tpool->head && 0 == tpool->quit) {
			gettimeofday(&now);
			tsp.tv_sec = now.tv_sec;
			tsp.tv_nsec = now.tv_usec * 1000;
			tsp.tv_sec += TIMEOUT;
			ret = pthread_cond_timedwait(&tpool->queue_ready,
					&tpool->queue_lock,
					&tsp);
			if(ETIMEDOUT == ret) {
				fprintf(stderr,
					"[%s]thread %lu wait timeout\n",
					__FUNCTION__, pthread_self());
				timeout = 1;
				break;
			}
		}

		tpool->idle--;

		if(tpool->head != NULL) {
			work = tpool->head;
			tpool->head = work->next;
			pthread_mutex_unlock(&tpool->queue_lock);
			work->routine(work->arg);
			free(work);
			pthread_mutex_lock(&tpool->queue_lock);
		}

		if(NULL == tpool->head && 1 == tpool->quit) {
			tpool->counter--;
			if(0 == tpool->counter)
				pthread_cond_signal(&tpool->queue_ready);
			pthread_mutex_unlock(&tpool->queue_lock);
			break;
		}

		if(NULL == tpool->head && 1 == timeout) {
			tpool->counter--;
			pthread_mutex_unlock(&tpool->queue_lock);
			break;
		}
		pthread_mutex_unlock(&tpool->queue_lock);
	}

	return (void *)0;
}
 
/*
* 创建线程池 
*/
int tpool_init(int max_thread)
{
	int i;
	 
	tpool = malloc(sizeof(tpool_t));
	if (NULL == tpool) {
		printf("%s: malloc failed\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}
	 
	/* 初始化 */
	tpool->quit = 0;
	tpool->idle = 0;
	tpool->counter = 0;
	tpool->max_thread = max_thread;
	tpool->head = NULL;
	tpool->tail = NULL;
	if (pthread_mutex_init(&tpool->queue_lock, NULL) !=0) {
		printf("[%s]: %s\n", __FUNCTION__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pthread_cond_init(&tpool->queue_ready, NULL) !=0 ) {
		printf("[%s]: %s\n", __FUNCTION__, strerror(errno));
		exit(EXIT_FAILURE);
	}
 
	return 0;
}
 
/* 销毁线程池 */
void tpool_destroy()
{
	int i;
	tpool_work_t *member;
	 
	if (1 == tpool->quit) {
		return;
	}
	tpool->quit = 1;
	 
	/* 通知所有正在等待的线程 */
	pthread_cond_broadcast(&tpool->queue_ready);
	 
	pthread_mutex_lock(&tpool->queue_lock);
	while(tpool->counter > 0) {
		pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
	}
	pthread_mutex_unlock(&tpool->queue_lock);

	pthread_mutex_destroy(&tpool->queue_lock);    
	pthread_cond_destroy(&tpool->queue_ready);
	free(tpool);
}
 
/* 向线程池添加任务 */
int tpool_add_work(void*(*routine)(void*), void *arg)
{
	int ret;
	tpool_work_t *work = NULL, *member = NULL;
	 
	if (!routine){
		printf("%s:Invalid argument\n", __FUNCTION__);
		return -1;
	}
	 
	work = malloc(sizeof(tpool_work_t));
	if (!work) {
		printf("%s:malloc failed\n", __FUNCTION__);
		return -1;
	}
	work->routine = routine;
	work->arg = arg;
	work->next = NULL;
	 
	pthread_mutex_lock(&tpool->queue_lock);    
	member = tpool->head;
	if (NULL == member) {
		tpool->head = work;
	} else {
		tpool->tail->next = work;
	}

	tpool->tail = work;

	if(tpool->idle > 0) {
		pthread_cond_signal(&tpool->queue_ready);
	} else if(tpool->counter < tpool->max_thread){
		tpool->counter++;
		PTHREAD_DETACH_CREATE(thread_routine, NULL)
	} else {
		fprintf(stderr, "The max thread has created\n");
	}
	pthread_mutex_unlock(&tpool->queue_lock);
	 
	return 0;    
}
