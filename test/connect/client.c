#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * 设置套接字的地址
 */
int addr_set(struct sockaddr_in *addr, socklen_t addlen,
		char *ip, short port)
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

	socket_non_block(sockfd);

	return sockfd;
}

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
 * 连接服务器，并发送登录报文
 */
int socket_conect(int sockfd, char *ip, short port)
{
	struct sockaddr_in servaddr;
	int ret;
	fd_set fds;
	int status, len;

	ret = addr_set(&servaddr, sizeof(servaddr), ip, port);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = connect(sockfd, (struct sockaddr *)&servaddr,
			sizeof(servaddr));

	if(-1 == ret) {
		while(1) {
			FD_ZERO(&fds);
			FD_SET(sockfd, &fds);
			select(sockfd + 1, NULL, &fds, NULL, NULL);
			if(FD_ISSET(sockfd, &fds)) {
				getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &status, &len);
				if(0 == status) {
					printf("Connected\n");
					break;
				}
			} else {
				printf("Sockfd cann't read\n");
			}
		}
	}

	return 0;

err_ret:
	fprintf(stderr, "[socket_conect]: %s\n",
		strerror(errno));
	return -1;
}


int main(int argc,char *argv[])
{
	int sockfd;
	sockfd = client_init();
	socket_conect(sockfd, "127.0.0.1", 8084);
	return 0;
}
