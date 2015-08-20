#include "client.h"

/*
 * 设置客户端的地址，并进行创建
 *
 * @ret:	sockfd
 */
int socket_client_create()
{
}

/*
 * 连接服务器，并发送登录报文
 */
int socket_conect()
{
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
	packet_make()
}

/*
 * 接收数据包或者响应包
 */
int packet_recv(int sockfd, struct head *phead)
{
	socket_recvn(HEAD_LEN)
	//判断包的类型
	socket_recvn(DATA_LEN)
	or
	socket_recvn(RESPONSE_LEN - HEAD_LEN);
}

int main(int argc,char *argv[])
{
	return 0;
}
