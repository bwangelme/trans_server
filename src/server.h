#ifndef SERVER_H
#define SERVER_H

#include "data_type.h"
#include <map>
#include <vector>
using namespace std;

#define MAX_CLIENT 10240	//服务器能够接受的客户端的最大连接数
#define MAX_EVENTS 20		//epoll listen event number

#define SOCKET_NULL 0xee000001  //套接字读空了
#define LIST_NULL 0xee000002	//用户的当前链表为空

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

struct s_key {
	u16 cid;
	bool operator <(const struct s_key &other)const
	{
		if(cid < other.cid)
			return true;
		else
			return false;
	}
};

struct data_node {
	struct data_packet *pack;
	struct data_node *next;
	struct data_node *prev;
};

struct data_list {
	pthread_mutex_t lock;
	struct data_node *head;
	struct data_node *tail;
	int count;
};

struct s_value {
	int fd;			//套接字描述符
	u8 online;		//用户在线的标志
	struct data_list list;	//存储用户数据的列表
};

map<struct s_key, struct s_value> usermap;	//用来存储cid和用户的映射
map<int, u16> f2c;				//用来存储cid和fd的映射

#endif /* server.h */
