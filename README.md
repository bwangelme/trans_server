转发服务器
==========

一个基于epoll和线程池的转发服务器，线程池利用Linux多线程和条件锁实现

##编译说明

make server 生成服务端程序
编译服务器程序时需要将thread\_pool/libthread\_pool.so 放到自定义的库位置
	/usr/local/lib/my\_lib/中，并设置好运行时的链接路径

make client 生成客户端程序

##代码结构说明

src 中存放着服务器和客户端代码及相关头文件
test 存放着自己写的小的测试程序
thread\_pool 存放着线程池的相关代码，线程池最终是以库的形式提供的
