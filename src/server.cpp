/* 	对象包含示意图
 *
 *   server_epoll
 *        \
 *        \ --- 控制
 *        \                     Receive function
 *	server(o) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *	  | -------  |  ------ | -------- |		    |
 *     usermap(go) f2c(go)   addr   threadpool(o)	    |
 *	  |	     ^					    |
 *	  |	     |					    |
 *	user(o) ~~~~~+ 影响				    |
 *	  |						    |
 *	  | ---------- | ------ | 			    |
 *    packet_list(o)  fd     online			    |
 *	  |						    |
 *	packet(o)					    |
 *	  |						    |
 *	socket(o) <~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
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
	struct sockaddr_in add;
	socklen_t len = sizeof(add);
	int connect_fd;
	char ipstr[16];

	bzero(&add, sizeof(add));
	bzero(ipstr, 16);
	connect_fd = accept(listenfd, (struct sockaddr *)&add, &len);
	if(-1 == connect_fd) {
		fprintf(stderr, "[socket_accept]: %s\n", strerror(errno));
		return -1;
	}
	printf("%s %d connected\n",
		inet_ntop(AF_INET, &add.sin_addr, ipstr, sizeof(add)),
		ntohs(add.sin_port));

	return connect_fd;
}

/*
 * 从套接字中读取指定长度的报文
 *
 * @ret:	读取的字节长度，如果刚开始就出错，返回-1
 *
 * notice:	non-blcok socket will meet EAGAIN error
 */
ssize_t socket_recvn(int sockfd, void *ptr, size_t len)
{
	size_t nleft, nrecv;
	unsigned char *vptr = NULL;

	vptr = (unsigned char *)ptr;
	nleft = len;

	while(nleft > 0) {
		nrecv = recv(sockfd, vptr, nleft, 0);
		if(-1 == nrecv) {
			if(EAGAIN != errno)
				fprintf(stderr, "[socket_recvn]: %s\n",
					strerror(errno));
			if(nleft == len) {
				if(EAGAIN == errno)
					return SOCKET_NULL;
				else
					return -1;
			} else {
				return (len - nleft);
			}
		} else if(0 == nrecv) {
			fprintf(stderr, "[socket_recvn]: %s\n",
				"The peer socket has closed");
			return (len - nleft);
		}

		vptr += nrecv;
		nleft -= nrecv;
	}

	return (len - nleft);
}

/*
 * 向某个套接字发送指定长度的报文
 */
ssize_t socket_sendn(int sockfd, void *ptr, size_t len)
{
	ssize_t nsend, nleft;
	unsigned char *vptr = NULL;

	nleft = len;
	vptr = (unsigned char *)ptr;

	while(nleft > 0) {
		nsend = send(sockfd, vptr, nleft, 0);
		if(-1 == nsend) {
			fprintf(stderr, "[socket_sendn]: %s\n",
				strerror(errno));
			if(nleft == len)
				return -1;
			else
				return (len - nleft);
		} else if(0 == nsend) {		/* EOF */
			fprintf(stderr, "[socket_sendn]: %s\n",
				"The peer socket has closed");
			return (len - nleft);
		}

		nleft -= nsend;
		vptr += nsend;
	}

	return (len - nleft);
}

/*
 * 关闭一个指定的套接字
 */
int socket_close(int sockfd)
{
}

/*
 * 设置一个指定类型的包
 */
int packet_make(struct head *phead, u32 type)
{
	struct response_packet *pres;

	switch(type) {
	case TYPE_RESPONSE:
		pres = (struct response_packet *)phead;
		pres->status = STATUS_LOGIN;
		pres->head.type = TYPE_RESPONSE;
		pres->head.dcid = 0;
		pres->head.scid = 0;
		pres->head.len = RESPONSE_LEN;
		break;
	default:
		fprintf(stderr, "[packet_make]: %s\n",
			"Packet type error");
		return -1;
	}
}

/*
 * 接收指定类型的报文
 *
 * @oplen:	可选的，除去头之后，附加内容的长度
 */
int packet_recv(int sockfd, struct head *phead, u32 type, int oplen = 0)
{
	int len;

	switch(type) {
	case TYPE_LOGIN:
		len = HEAD_LEN;
		break;
	case TYPE_DATA:
		len = oplen;
		break;
	default:
		fprintf(stderr, "[packet_recv]: %s\n",
			"Invalid packet type");
		return -1;
	}

	if(len != socket_recvn(sockfd, phead, len)) {
		fprintf(stderr, "[packet_recv]\n");
		return -1;
	}

	return 0;
}

/*
 * 判断包的类型，发送相应长度的报文
 *
 * @sockfd:	套接字描述符
 * @ret:	发送的字节长度
 */
int packet_send(int sockfd, struct head *phead)
{
	int len;

	switch(phead->type) {
	case TYPE_RESPONSE:
		len = socket_sendn(sockfd, phead, RESPONSE_LEN);
		if(len != RESPONSE_LEN) {
			fprintf(stderr, "[packet_send]\n");
			return -1;
		}
		break;
	case TYPE_DATA:
		break;
	default:
		fprintf(stderr, "[packet_send]: %s\n",
			"Invalid packet type");
		return -1;
	}

	return len;
}

/*
 * 初始化数据包列表
 */
int list_init(struct data_list *plist)
{
	plist->lock = PTHREAD_MUTEX_INITIALIZER;
	plist->head = NULL;
	plist->tail = NULL;
	plist->count = 0;

	return 0;
}

/*
 * 向一个数据链表中添加数据报文
 */
int list_add(struct s_key key, struct data_packet *pdata)
{
	struct data_node *pnode = NULL;
	pnode = (struct data_node *)malloc(sizeof(struct data_node));

	pnode->pack = pdata;
	pnode->next = NULL;

	pthread_mutex_lock(&usermap[key].list.lock);
	if(NULL == usermap[key].list.head && 
		usermap[key].list.tail == NULL) {
		usermap[key].list.head = pnode;
		usermap[key].list.tail = pnode;
		pnode->prev = NULL;
	} else {
		pnode->prev = usermap[key].list.tail;
		usermap[key].list.tail->next = pnode;
		usermap[key].list.tail = pnode;
	}
	printf("Add packet %d to %d\n", usermap[key].list.tail->pack->head.dcid, key.cid);
	pthread_mutex_unlock(&usermap[key].list.lock);
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
 * 向f2c数组中添加fd -- cid对
 */
int f2c_add(int sockfd, u16 cid)
{
	auto iter = f2c.find(sockfd);
	if(iter == f2c.end()) {	/* 该fd不存在 */
		auto ret = f2c.insert(make_pair(sockfd, cid));
		if(!ret.second) {
			fprintf(stderr, "[f2c.insert]\n");
			goto err_ret;
		}
	} else {
		goto err_ret;
	}

	return 0;
err_ret:
	fprintf(stderr, "[f2c_add]\n");
	return -1;
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
u16 f2c_query(int sockfd)
{
	return f2c[sockfd];
}

/*
 * 对用户进行初始化
 *
 */
int user_init(struct s_value *pvalue)
{
	int ret;

	ret = list_init(&pvalue->list);
	if(-1 == ret) {
		fprintf(stderr, "[user_init]\n");
		return -1;
	}

	return 0;
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
 * 将相应用户的在线标志设置为1,设置上用户的套接字描述符
 * 同时也将fd和cid的映射添加到f2c中
 */
int user_login(struct s_key key, int sockfd, u32 cid)
{
	int ret;

	auto iter = usermap.find(key);
	if(iter != usermap.end()) {
		iter->second.online = 1;
		iter->second.fd = sockfd;
	} else {
		fprintf(stderr, "[user_login]: %s\n",
			"Cann't find the key");
		return -1;
	}

	ret = f2c_add(sockfd, cid);
	if(-1 == ret) {
		fprintf(stderr, "[user_login]\n");
		return -1;
	}

	printf("%d -- %d Login\n", usermap[key].fd, key.cid);
	return 0;
}

/*
 * 向用户的数据包列表中添加数据包
 */
int user_data_push(struct s_key key, struct data_packet *pdata)
{
	list_add(key, pdata);
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
 * 向usermap中添加新的用户
 */
int usermap_add_user(int sockfd, u16 cid)
{
	struct s_key key = { cid };
	struct s_value value;
	int ret;

	auto iter = usermap.find(key);
	if(iter == usermap.end()) {	/* 用户不存在 */
		ret = user_init(&value);
		if(-1 == ret) {
		//这里不能用goto是因为goto后面不能有新的变量定义
			fprintf(stderr, "[usermap_add_user]\n");
			return -1;
		}

		auto mret = usermap.insert(make_pair(key, value));
		if(!mret.second) {
			fprintf(stderr, "[usermap.insert]\n");
			goto err_ret;
		}
	} else {
		fprintf(stdout, "[usermap_add_user]: %s\n",
			"User exist");
	}

	ret = user_login(key, sockfd, cid);
	if(-1 == ret)
		goto err_ret;

	return 0;

err_ret:
	fprintf(stderr, "[usermap_add_user]\n");
	return -1;
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
 * 服务器发送相应报文
 */
int server_response(int sockfd)
{
	struct response_packet response;	

	if(-1 == packet_make((struct head *)&response, TYPE_RESPONSE)) {
		fprintf(stderr, "[server_response]\n");
		return -1;
	}

	if(-1 == packet_send(sockfd, (struct head *)&response)) {
		fprintf(stderr, "[server_response]\n");
		return -1;
	}

	return 0;
}

/*
 * 服务器接收一个新的用户的登录
 */
int server_accept(int listenfd)
{
	struct login_packet packet;
	int sockfd;

	sockfd = socket_accept(listenfd);
	if(-1 == sockfd)
		goto err_ret;

	if(-1 == socket_non_block(sockfd)) {
		goto err_ret;
	}

	if(-1 == packet_recv(sockfd, (struct head *)&packet, TYPE_LOGIN))
		goto err_ret;

	if(-1 == usermap_add_user(sockfd, packet.head.scid))
		goto err_ret;

	if(-1 == server_response(sockfd)) {
		goto err_ret;
	}

	return sockfd;
err_ret:
	fprintf(stderr, "[server_accept]\n");
	return -1;
}

/*
 * 服务器关闭与当前用户的连接
 */
int server_close()
{
	/* user_logout() */
}

/*
 * 1. 服务器从用户那里接收[数据]报文并存储到用户数据包列表中
 * or
 * 2. 接收到用户[退出]报文，关闭与这个用户的连接
 *
 */
void *server_receive(void *arg)
{
	int sockfd = *(int *)arg;
	struct head *phead = NULL;
	struct s_key key;
	int len;

	phead = (struct head *)malloc(BUF_LEN);

	while(1) {

	len = socket_recvn(sockfd, phead, HEAD_LEN); /* Read a head */
	if(-1 == len) {
		fprintf(stderr, "[server_receive]\n");
		exit(EXIT_FAILURE);
	} else if(SOCKET_NULL == len) {
	/* socket buffer read null */
		printf("Read socket null\n");
		break;
	}

	if(TYPE_EXIT == phead->type) { /* Exit packet */
		printf("Receive the exit packet\n");
		server_close();
	} else if(TYPE_DATA == phead->type) { /* Data packet */
		printf("Receive %d bytes[%d]\n",
			phead->len - HEAD_LEN, phead->dcid);
		len = socket_recvn(sockfd,
			(unsigned char *)phead + HEAD_LEN,
			phead->len - HEAD_LEN);
		if(-1 == len) {
			fprintf(stderr, "[server_receive]: %s\n",
				"Read data failed\n");
			exit(EXIT_FAILURE);
		}
		key.cid = phead->dcid;
		user_data_push(key, (struct data_packet *)phead);
	} else {
		fprintf(stderr, "[server_receive]: %s\n",
			"Packet type error");
		exit(EXIT_FAILURE);
	}

	}
	free(phead);

	return (void *)0;
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
	int listenfd, sockfd;
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
			EPOLLIN | EPOLLET)) {  //采用边缘触发模式
		err = 1;
		goto free_sockfd;
	}

	for(;;) {

	n = server_epoll_wait(epollfd, events, MAX_EVENTS);
	for(i = 0; i < n; i++) {
		if(events[i].events & EPOLLERR ||
		   events[i].events & EPOLLHUP) {
			fprintf(stderr, "[main]: %d epoll error\n",
				events[i].data.fd);
			close(events[i].data.fd);
		} else if(events[i].data.fd == listenfd) {
			sockfd = server_accept(listenfd);
			if(-1 == sockfd) {
				err = 1;
				goto free_sockfd;
			}
			if(-1 == server_epoll_add(epollfd, sockfd,
				EPOLLIN | EPOLLOUT | EPOLLET)) {
				err = 1;
				goto free_sockfd;
			}
		} else {
			if(events[i].events & EPOLLIN) {
				PTHREAD_DETACH_CREATE(server_receive,
					&events[i].data.fd)
			} else if (events[i].events & EPOLLOUT) {
			}
		}
	}

	}

free_sockfd:
	close(epollfd);
	close(listenfd);
	if(1 == err)
		goto err_ret;
	return 0;

err_ret:
	fprintf(stderr, "[main]\n");
	exit(EXIT_FAILURE);
}
