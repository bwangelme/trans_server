#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include "common.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct head {
	u32 type;
	u16 dcid;
	u16 scid;
	u32 len;
};

struct login_packet {
	struct head head;
};

struct data_packet {
	struct head head;
	char data[0];
};

struct exit_packet {
	struct head head;
};

struct response_packet {
	struct head head;
	u32 status;
};

#define HEAD_LEN sizeof(struct head)
/* #define DATA_LEN 1024 * 1024 * 5 */
#define DATA_LEN 1024
#define BUF_LEN HEAD_LEN+DATA_LEN
#define RESPONSE_LEN HEAD_LEN+sizeof(u32)
#define TIMEOUT 20

#define STATUS_LOGIN	0x00000011  //登录成功

#define TYPE_LOGIN 	0xff000001
#define TYPE_DATA  	0xff000002
#define TYPE_EXIT  	0xff000003
#define TYPE_RESPONSE   0xff000004

#endif
