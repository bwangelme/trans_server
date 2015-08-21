/*
 *    对象关系示意图
 *
 *     client  ----------------------------  user
 *        |-------|------|
 *      packet    |     addr
 *        |--s-r--|
 *      发送和接收|
 *              socket
 */

#include "client.h"

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
		fprintf(stderr, "[addr_set]: %s\n",
			strerror(errno));
		return -1;
	} else if(0 == ret) {
		fprintf(stderr, "[addr_set]: %s\n",
			"ip string invalied");
		return -1;
	}

	return 0;
}

/*
 * 设置客户端的地址，并进行创建
 *
 * @ret:	sockfd
 */
int socket_client_create()
{
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sockfd) {
		fprintf(stderr, "[socket_client_create]: %s\n", strerror(errno));
		return -1;
	}

	return sockfd;
}

/*
 * 连接服务器，并发送登录报文
 */
int socket_conect(int sockfd, char *ip, u16 port)
{
	struct sockaddr_in servaddr;
	int ret;

	ret = addr_set(&servaddr, sizeof(servaddr), ip, port);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = connect(sockfd, (struct sockaddr *)&servaddr,
			sizeof(servaddr));
	if(-1 == ret) {
		goto err_ret;
	}

	return 0;

err_ret:
	fprintf(stderr, "[socket_conect]: %s\n",
		strerror(errno));
	return -1;
}

/*
 * 向某个套接字发送指定长度的报文
 */
int socket_sendn()
{
}

/*
 * 从某个套接字接收指定长度的报文
 */
int socket_recvn()
{
}

/*
 * 制作登录包，数据包，退出包
 */
int packet_make()
{
}

/*
 * 判断包的类型，发送相应长度的报文
 *
 * @sockfd:	套接字描述符
 */
int packet_send(int sockfd, struct head *phead)
{
	/* packet_make() */
}

/*
 * 接收数据包或者响应包
 */
int packet_recv(int sockfd, struct head *phead)
{
	/* socket_recvn(HEAD_LEN) */
	/* //判断包的类型 */
	/* socket_recvn(DATA_LEN) */
	/* or */
	/* socket_recvn(RESPONSE_LEN - HEAD_LEN); */
}

/*
 * 客户端进行初始化，创建连接套接字
 *
 * @ret: sockfd
 */
int client_init()
{
	int sockfd;

	sockfd = socket_client_create();
	if(-1 == sockfd) {
		fprintf(stderr, "[client_init]\n");
		return -1;
	}

	return sockfd;
}

/*
 * 客户端进行登录，连接并发送登录报文
 */
int client_login(int sockfd, char *ip, u16 port)
{
	int ret;
	ret = socket_conect(sockfd, ip, port);
	if(-1 == ret) {
		fprintf(stderr, "[client_login]\n");
		return -1;
	}

	while(1) {}
	/* ret = packet_make() */
	/* ret = packet_send() */

	return 0;
}

/*
 * 客户端进行数据的接收
 */
int client_recv()
{
}

/*
 * 客户端进行数据的发送
 */
int client_send()
{
}

/*
 * 客户端发送退出报文，并关闭套接字
 */
int client_logout()
{
}

int main(int argc,char *argv[])
{
	int sockfd;
	int ret;

	sockfd = client_init();
	if(-1 == sockfd)
		goto err_ret;

	ret = client_login(sockfd, (char *)IP, PORT);
	if(-1 == ret)
		goto free_sock;

	return 0;

free_sock:
	close(sockfd);
err_ret:
	fprintf(stderr, "[main]\n");
	exit(EXIT_FAILURE);
}
