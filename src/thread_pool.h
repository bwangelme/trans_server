#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__
 
#include <pthread.h>

#define TIMEOUT  61	/* 设置线程的休眠时延，以秒为单位 */
typedef struct tpool_work {
    void*               (*routine)(void*);       /* 任务函数 */
    void                *arg;                    /* 传入任务函数的参数 */
    struct tpool_work   *next;                    
}tpool_work_t;
 
typedef struct tpool {
    int             quit;                    	/* 线程池是否销毁 */
    int	            idle;			/* 空闲线程数 */
    int             counter;                	/* 线程数 */
    int             max_thread;			/* 最大线程数 */
    tpool_work_t    *head;                	/* 线程链表头 */
    tpool_work_t    *tail;                	/* 线程链表尾 */
    pthread_mutex_t queue_lock;
    pthread_cond_t  queue_ready;
}tpool_t;
 
#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief     创建线程池 
 * @param     max_thr_num 最大线程数
 * @return     0: 成功 其他: 失败  
 */
int tpool_init(int max_thr_num);
 
/*
 * @brief     销毁线程池 
 */
void tpool_destroy();
 
/*
 * @brief     向线程池中添加任务
 * @param    routine 任务函数指针
 * @param     arg 任务函数参数
 * @return     0: 成功 其他:失败 
 */
int tpool_add_work(void*(*routine)(void*), void *arg);
 
#ifdef __cplusplus
}
#endif
/*
 * 创建分离线程
 */
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

#endif
