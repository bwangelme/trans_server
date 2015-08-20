#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <map>
#include <vector>
using namespace std;

#define MAX_CLIENT 10240	//服务器能够接受的客户端的最大连接数
#define MAX_EVENTS 20		//epoll listen event number

struct s_key {
	u16 cid;
};

struct data_node {
	struct data_packet *pack;
	struct data_node *next;
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
vector<u16> f2c;				//用来存储cid和fd的映射

#endif /* server.h */
