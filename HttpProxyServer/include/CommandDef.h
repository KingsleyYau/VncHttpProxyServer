/*
 * CommandDef.h
 *
 *  Created on: 2014年3月6日
 *      Author: Kingsley Yau
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef CommandDef_H_
#define CommandDef_H_

#pragma pack(push,1)

#define MAX_PARAM_LEN 4096

/*
 * 命令类型
 */
typedef enum CommandType {
	CommandTypeNone = 0,
	CommandTypeProxy,
} CMDT;

/*
 * 公用命令头
 */
typedef struct CommandHedaer {
	CommandHedaer() {
		cmdt = CommandTypeNone;
		len = 0;
		seq = 0;
		bNew = false;
	}

	CommandHedaer(const CommandHedaer& item) {
		cmdt = item.cmdt;
		len = item.len;
		seq = item.seq;
		bNew = item.bNew;
	}

	CommandHedaer operator=(const CommandHedaer& item) {
		cmdt = item.cmdt;
		len = item.len;
		seq = item.seq;
		bNew = item.bNew;
		return *this;
	}

	CMDT cmdt;			// 类型
	int len;			// 参数长度
	int seq;			// 请求号
	bool bNew;			// 主动发起请求
} CMDH;

/*
 * 命令
 */
typedef struct Command {
	CMDH header;					// 命令头
	char param[MAX_PARAM_LEN];		// 参数
} CMD;

typedef enum ProtocolType {
	HTML,
	JSON,
} PROTOCOLTYPE;

#pragma pack(pop)

#endif /* CommandDef_H_ */
