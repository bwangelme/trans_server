/* 	对象包含示意图
 *
 *   server_epoll
 *        \
 *        \ --- 控制
 *        \
 *	server(o)
 *	  | -------  |  ------ | -------- |
 *     usermap(go) f2c(go)   addr   threadpool(o)
 *	  |	       
 *	user(o)
 *	  | ---------- | ------ | 
 *    packet_list(o)  fd     online
 *	  |
 *	packet(o)
 *	  |
 *	socket(o)
 *
 * usermap: 用户和cid的映射，为了以后添加新的键元素，所以用结构体来保存键
 * f2c:	fd到cid的映射
 * epoll: epoll的描述符
 *
 * g: 全局变量
 * o: 对象
 */

#include "server.h"

/*
 * 设置套接字描述符为非阻塞
 */
int socket_non_block(int sockfd)
{
	int flags, ret;

	flags = fcntl(sockfd, F_GETFL, 0);
	if(-1 == flags) {
		goto err_ret;
	}

	flags |= O_NONBLOCK;

	ret = fcntl(sockfd, F_SETFL, flags);
	if(-1 == ret) {
		goto err_ret;
	}

	return 0;

err_ret:
	fprintf(stderr, "[socket_non_block]: %s\n", strerror(errno));
	return -1;
}

/*
 * 创建服务器套接字，并进行绑定监听,将描述符设置为非阻塞
 *
 * @ret:	监听套接字
 */
int socket_server_create(struct sockaddr *addr, socklen_t addrlen)
{
	int listenfd;
	int ret;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == listenfd) {
		goto ret;
	}

	ret = bind(listenfd, addr, addrlen);
	if(-1 == ret) {
		goto free_ret;
	}

	ret = listen(listenfd, MAX_CLIENT);
	if(-1 == ret) {
		goto free_ret;
	}

	ret = socket_non_block(listenfd);
	if(-1 == ret) {
		goto free_ret;
	}

	return listenfd;

free_ret:
	close(listenfd);
ret:
	fprintf(stderr, "[socket_server_create]: %s\n", strerror(errno));
	return -1;
}

/*
 * 接收连接，返回连接套接字
 *
 * @ret:	connect_fd
 */
int socket_accept(int listenfd)
{
}

/*
 * 向套接字发送指定长度的报文
 */
int socket_recvn()
{
}

/*
 * 从套接字中读取指定长度的报文
 */
int socket_sendn()
{
}

/*
 * 关闭一个指定的套接字
 */
int socket_close(int sockfd)
{
}

/*
 * 接收报文
 */
int packet_recv(struct head *phead)
{
	//首先读个头，然后再按照头中的类型进行相应的读取
	socket_recvn();
}

/*
 * 发送报文
 */
int packet_send(struct head *phead)
{
	//判断头的类型，然后再进行相应的发送
	/* socket_sendn(); */
}

/*
 * 初始化数据包列表
 */
int list_init()
{
	/* lock = ; */
	/* head = ; */
	/* count = 0; */
}

/*
 * 向一个数据链表中添加数据报文
 */
int list_add()
{
}

/*
 * 从一个数据链表中弹出数据报文并发送
 */
int list_pop()
{
	/* packet_send(); */
	//释放数据报文的空间
}

/*
 * 对用户进行初始化
 *
 * @sockfd:	这个用户所对应的套接字描述符
 */
int user_init(int sockfd)
{
	/* fd =sockfd; */
	/* list_init() */
}

/*
 * 用户离线同时也将f2c中关于fd的映射取消掉
 */
int user_logout()
{
	/* online = 0; */
	/* f2c_delete(); */
}

/*
 * 将相应用户的在线标志设置为1同时也将fd和cid的映射添加到f2c中
 */
int user_login()
{
	/* f2c_add(); */
}

/*
 * 向用户的数据包列表中添加数据包
 */
int user_data_add()
{
	/* list_add() */
}

/*
 * 用户返回其数据包列表中的一个包
 */
int user_data_pop()
{
	/* list_pop() */
}

/*
 * 判断一个用户是否在线
 */
int user_is_onlie()
{
}

/*
 * 向cmap中添加新的用户
 */
int usermap_add_user()
{
	/* user_init(); */
}

/*
 * 向f2c数组中添加fd -- cid对
 */
int f2c_add(int fd, u16 cid)
{
}

/*
 * 将f2c数组中的fd -- cid对删除
 *
 * 即将fd对应的值设置为-1
 */
int f2c_delete(int fd)
{
}

/*
 * 给出fd，查询f2c数组中的cid
 *
 * @ret:	cid
 */
int f2c_query(int fd)
{
}

/*
 * 创建epoll监听描述符
 *
 * @ret:	epollfd
 */
int server_epoll_create()
{
	int epollfd;

	epollfd = epoll_create(1);

	if(-1 == epollfd) {
		close(epollfd);
		fprintf(stderr, "[my_epoll_create]: %s\n",
			strerror(errno));
		return -1;
	}

	return epollfd;
}

/*
 * 向epoll中添加文件描述符，并设定监听的事件
 *
 *
 */
int server_epoll_add(int epollfd, int fd, u32 events)
{
	int ret;
	struct epoll_event event;

	event.data.fd = fd;
	event.events = events;
	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

	if(-1 == ret) {
		close(epollfd);
		fprintf(stderr, "[server_epoll_add]: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

/*
 * 等待epoll事件
 */
int server_epoll_wait(int epollfd, struct epoll_event *events,
			int max_events)
{
	int n;

	n = epoll_wait(epollfd, events, max_events, -1);

	if(-1 == n) {
		close(epollfd);
		fprintf(stderr, "[server_epoll_wait]: %s\n",
				strerror(errno));
		return -1;
	}

	return n;
}

/*
 * 设置套接字的地址
 */
int addr_set(struct sockaddr_in *addr, socklen_t addlen,
		char *ip, u16 port)
{
	int ret;

	bzero(addr, addlen);
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	ret = inet_pton(AF_INET, ip, &addr->sin_addr);

	if(-1 == ret) {
		fprintf(stderr, "[server_set_addr]: %s\n",
			strerror(errno));
		return -1;
	} else if(0 == ret) {
		fprintf(stderr, "[server_set_addr]: %s\n",
			"ip string invalied");
		return -1;
	}

	return 0;
}

/*
 * 服务器初始化，创建套接字
 */
int server_init(char *ip, u16 port)
{
	int listenfd, ret;
	struct sockaddr_in servaddr;

	ret = addr_set(&servaddr, sizeof(servaddr), ip, port);
	if(-1 == ret) {
		goto err_ret;
	}

	listenfd = socket_server_create((struct sockaddr *)&servaddr,
			sizeof(servaddr));
	if(-1 == listenfd) {
		goto err_ret;
	}

//TODO:
//	threadpool_init();

	return listenfd;

err_ret:
	fprintf(stderr, "[server_init]\n");
	return -1;
}

/*
 * 服务器接收一个新的用户的登录
 */
int server_accept()
{
	/* socket_accept(); */
	/* packet_recv(login_packet); */
	/* cmap_add_user(); */
	/* user_login(); */
}

/*
 * 服务器从用户那里接收数据报文并存储到用户数据包列表中
 */
int server_recvive()
{
	//申请数据报文的空间
	/* packet_recv(); */

	//如果发送过来的是退出报文，则user_logout
	/* if(type = exit) */
	/* 	user_logout() */
	/* else */
	/* 	user_data_add(); */
}

/*
 * 服务器发送报文到用户
 *
 * 根据传送过来的描述符，判断用户是否在线，如果在线则发送
 */
int server_send()
{
	if(user_is_onlie())
		user_data_pop();
}

int main(int argc, char *argv[])
{
	int listenfd;
	int epollfd;
	int ret, n, i, err = 0;
	struct epoll_event events[MAX_EVENTS];

	bzero(events, MAX_EVENTS * sizeof(struct epoll_event));

	listenfd = server_init((char *)IP, PORT);
	if(-1 == listenfd) {
		err = 1;
		goto err_ret;
	}

	epollfd = server_epoll_create();
	if(-1 == epollfd) {
		err = 1;
		goto free_sockfd;
	}

	if(-1 == server_epoll_add(epollfd, listenfd,
			EPOLLIN | EPOLLET)) {
		err = 1;
		goto free_sockfd;
	}

	for(;;) {
		n = server_epoll_wait(epollfd, events, MAX_EVENTS);
		for(i = 0; i < n; i++) {
			if( (events[i].events & EPOLLERR) ||
			    (events[i].events & EPOLLHUP) ||
			    (!(events[i].events & EPOLLIN)) ) {
				fprintf(stderr, "[main]: %d epoll error",
					events[i].data.fd);
				close(events[i].data.fd);
			} else if(events[i].data.fd == listenfd) {
				printf("Accept -- \n");
				/* server_accept(); */
			} else {
				if(events[i].events & EPOLLIN) {
					printf("Read from %d\n",
						events[i].data.fd);
				} else if (events[i].events & EPOLLOUT) {
					printf("Write to %d\n",
						events[i].data.fd);
				}
			}
		}
	}


	close(epollfd);
free_sockfd:
	close(listenfd);
	if(1 == err)
		goto err_ret;
	return 0;

err_ret:
	fprintf(stderr, "[main]\n");
	exit(EXIT_FAILURE);
}
