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
 * 从套接字中读取指定长度的报文
 *
 * @ret:	读取的字节长度，如果刚开始就出错，返回-1
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
			fprintf(stderr, "[socket_recvn]: %s\n",
				strerror(errno));
			if(nleft == len) {
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
 * 制作登录包，数据包，退出包
 */
int packet_make(struct head *head, u16 scid, u32 type, u16 dcid)
{
	struct login_packet *lpack = NULL;
	struct data_packet *dpack = NULL;
	struct exit_packet *epack = NULL;

	switch(type) {
	case TYPE_LOGIN:
		lpack = (struct login_packet *)head;
		lpack->head.type = type;
		lpack->head.dcid = dcid;
		lpack->head.scid = scid;
		lpack->head.len = HEAD_LEN;
		break;
	case TYPE_DATA:
		dpack = (struct data_packet *)head;
		bzero(dpack, BUF_LEN);
		dpack->head.type = type;
		dpack->head.dcid = dcid;
		dpack->head.scid = scid;
		dpack->head.len = BUF_LEN;
		break;
	case TYPE_EXIT:
		epack = (struct exit_packet *)head;
		epack->head.type = type;
		epack->head.dcid = dcid;
		epack->head.scid = scid;
		epack->head.len = HEAD_LEN;
		break;
	default:
		fprintf(stderr, "[packet_make]: %s\n", "invailed type");
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
	case TYPE_EXIT:
	case TYPE_LOGIN:
		len = socket_sendn(sockfd, phead, HEAD_LEN);
		if(len != HEAD_LEN) {
			fprintf(stderr, "[packet_send]\n");
			return -1;
		}
		break;
	case TYPE_DATA:
		len = socket_sendn(sockfd, phead, phead->len);
		if(len != phead->len) {
			fprintf(stderr, "[packet_send]\n");
			return -1;
		}
		break;
	default:
		fprintf(stderr, "[packet_send]: %s\n",
			"Invalid packet type");
		return -1;
	}

	return len;
}

/*
 * 接收指定类型的报文
 *
 */
int packet_recv(int sockfd, struct head *phead, u32 type)
{
	int len;

	switch(type) {
	case TYPE_RESPONSE:
		len = RESPONSE_LEN;
		break;
	case TYPE_DATA:
		len = BUF_LEN;
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

	return len;
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
int client_login(int sockfd, char *ip, u16 port, u16 scid)
{
	int ret;
	struct login_packet packet;
	struct response_packet resp;

	ret = socket_conect(sockfd, ip, port);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = packet_make((struct head*)&packet, scid, TYPE_LOGIN, 0);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = packet_send(sockfd, (struct head *)&packet);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = packet_recv(sockfd, (struct head *)&resp, TYPE_RESPONSE);
	if(-1 == ret) {
		goto err_ret;
	}
	
	if(STATUS_LOGIN == resp.status) {
		printf("%d Login success\n", scid);
		return 0;
	}

err_ret:
	fprintf(stderr, "[client_login]\n");
	return -1;
}

/*
 * 客户端进行数据的接收
 */
void *client_recv(void *arg)
{
	int sockfd = *(int *)arg;
	struct data_packet *pdata = NULL;
	int len = 1, ret;

	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(0 != ret) {
		fprintf(stderr, "[client_recv]: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if(0 != ret) {
		fprintf(stderr, "[client_recv]: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	pthread_key_create(&r_key, free);
	if(NULL == pthread_getspecific(r_key)) {
		pdata = (struct data_packet *)malloc(BUF_LEN);
		pthread_setspecific(r_key, pdata);
	}

	while(len != 0) {
		len = packet_recv(sockfd,
				(struct head *)pdata, TYPE_DATA);
		if(-1 == len) {
			fprintf(stderr, "[client_recv]\n");
			exit(-1);
		}
		printf("Recv %d bytes from the %d\n",
			len, pdata->head.scid);
	}

	/* 理论上这里不会被执行 */
	free(pdata);
	pdata = NULL;

	return (void *)3;
}

/*
 * 客户端进行数据的发送
 */
void *client_send(void *arg)
{
	int sockfd = *(int *)arg;
	struct data_packet *pdata = NULL;
	int sendlen;
	int i;
	int dcid;
	int packet_num;

	printf("Please enter the packet_num & dcid: ");
	while(EOF != scanf("%d%d", &packet_num, &dcid)) {

	//Send packet_num packets
	pdata = (struct data_packet *)malloc(BUF_LEN);
	for(i = 0; i < packet_num; i++) {
		/* dcid = 1000 - scid - i + 1; */
		if(-1 == packet_make((struct head *)pdata,
				scid, TYPE_DATA, dcid)) {
			goto err_ret;
		}
		sendlen = packet_send(sockfd, (struct head *)pdata);
		if(-1 == sendlen) {
			goto err_ret;
		} else {
			printf("Send %d bytes to %d\n", sendlen, dcid);
		}
	}
	free(pdata);

	printf("Please enter the packet_num & dcid: ");
	}

	return (void *)2;

err_ret:
	fprintf(stderr, "[client_send]\n");
	return (void *)-1;
}

/*
 * 客户端发送退出报文，并关闭套接字
 */
int client_logout(int sockfd, u16 scid)
{
	struct exit_packet pexit;
	int ret;

	ret = packet_make((struct head *)&pexit, scid, TYPE_EXIT, 0);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = packet_send(sockfd, (struct head *)&pexit);
	if(-1 == ret) {
		goto err_ret;
	}

	ret = read(sockfd, NULL, 0);
	if(0 == ret) {
		printf("User %d logout\n", scid);
		scid = -1;
	} else {
		goto err_ret;
	}

	return 0;

err_ret:
	fprintf(stderr, "[client_logout]\n");
	return -1;
}

int main(int argc,char *argv[])
{
	int sockfd;
	int ret;
	pthread_t rtid, stid;
	void *tret;

	if(argc != 2) {
		fprintf(stdout, "Usage: %s <scid>\n", argv[0]);
		return 0;
	}

	sscanf(argv[1], "%hu", &scid);

	ret = client_init();
	if(-1 == ret)
		goto err_ret;
	sockfd = ret;

	ret = client_login(sockfd, (char *)IP, PORT, scid);
	if(-1 == ret)
		goto free_sock;

	ret = pthread_create(&stid, NULL, client_send, &sockfd);
	if(ret != 0) {
		goto free_sock;
	}
	ret = pthread_create(&rtid, NULL, client_recv, &sockfd);
	if(ret != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto free_sock;
	}

	ret = pthread_join(stid, &tret);
	if(ret != 0) {
		goto free_sock;
	} else {
		printf("Send thread quit with code %ld\n", (long)tret);
	}

	if(2 == (long)tret) {
		if(-1 == client_logout(sockfd, scid))
			goto free_sock;
		ret = pthread_cancel(rtid);
		if(0 != ret) {
			fprintf(stderr, "%s\n", strerror(ret));
			goto free_sock;
		}
	} else {
		goto free_sock;
	}

	ret = pthread_join(rtid, &tret);
	if(ret != 0) {
		goto free_sock;
	} else {
		printf("Receive thread quit with code %ld\n", (long)tret);
	}

free_sock:
	close(sockfd);
err_ret:
	if(ret != 0)
		fprintf(stderr, "[main]: %s\n", strerror(ret));
	return ret;
}
