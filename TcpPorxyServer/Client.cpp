/*
 * Client.cpp
 *
 *  Created on: 2015-11-10
 *      Author: Max
 */

#include "Client.h"

Client::Client() {
	// TODO Auto-generated constructor stub
	fd = -1;
	isOnline = false;

    ip = "";

    mBuffer.Reset();

    miPacketSendSeq = 0;
    miDataPacketRecvSeq = 0;

	mpClientCallback = NULL;

	for(int i = 0; i < 4; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}
}

Client::~Client() {
	// TODO Auto-generated destructor stub

	while( true ) {
		Message* m = mIdleMessageList.PopFront();
		if( m != NULL ) {
			delete m;
		} else {
			break;
		}
	}
}

void Client::SetClientCallback(ClientCallback* pClientCallback) {
	mpClientCallback = pClientCallback;
}

int Client::ParseData(Message* m)  {
	int ret = 0;

	char* buffer = m->buffer;
	int len = m->len;
	int seq = m->seq;

	mKMutex.lock();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseData( "
			"tid : %d, "
			"[收到客户端数据包], "
			"miDataPacketRecvSeq : %d, "
			"seq : %d "
			")",
			(int)syscall(SYS_gettid),
			miDataPacketRecvSeq,
			seq
			);

	if( miDataPacketRecvSeq != seq ) {
		// 收到客户端乱序数据包
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseData( "
				"tid : %d, "
				"[收到客户端乱序的数据包] "
				")"
				);

		// 缓存到队列
		Message* mc = mIdleMessageList.PopFront();
		if( mc != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[缓存客户端乱序的数据包到队列] "
					")"
					);

			memcpy(mc->buffer, buffer, len);
			mc->len = len;
			mMessageMap.Insert(seq, mc);

			ret = 0;

		} else {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[客户端没有缓存空间] "
					")"
					);
			ret = -1;
		}

	} else {
		// 收到客户端顺序的数据包
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseData( "
				"tid : %d, "
				"[收到客户端顺序的数据包] "
				")",
				(int)syscall(SYS_gettid)
				);

		ret = 1;
	}

	// 开始解析数据包
	if( ret == 1 ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseData( "
				"tid : %d, "
				"[开始解析客户端当前数据包] "
				")",
				(int)syscall(SYS_gettid)
				);

		// 解当前数据包
		ret = ParseDataNoCache(m);
		if( ret != -1 ) {
			miDataPacketRecvSeq++;

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[开始解析客户端缓存数据包] "
					")",
					(int)syscall(SYS_gettid)
					);

			// 解析缓存数据包
			MessageMap::iterator itr;
			while( true ) {
				itr = mMessageMap.Find(miDataPacketRecvSeq);
				if( itr != mMessageMap.End() ) {
					// 找到对应缓存包
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"Client::ParseData( "
							"tid : %d, "
							"[找到对应缓存包], "
							"miDataPacketRecvSeq : %d "
							")",
							(int)syscall(SYS_gettid),
							miDataPacketRecvSeq
							);

					Message* mc = itr->second;
					ret = ParseDataNoCache(mc);

					mIdleMessageList.PushBack(mc);
					mMessageMap.Erase(itr);

					if( ret != -1 ) {
						miDataPacketRecvSeq++;

					} else {
						break;
					}

				} else {
					// 找不到对应缓存包
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"Client::ParseData( "
							"tid : %d, "
							"[找不到对应缓存包]"
							")",
							(int)syscall(SYS_gettid)
							);
					break;
				}
			}
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseData( "
			"tid : %d, "
			"[解析客户端数据包完成], "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			ret
			);

	mKMutex.unlock();

	return ret;
}

int Client::ParseDataNoCache(Message* m) {
	int ret = 0;

	char* buffer = m->buffer;
	int len = m->len;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseDataNoCache( "
			"tid : %d, "
			"miDataPacketRecvSeq : %d "
			")",
			(int)syscall(SYS_gettid),
			miDataPacketRecvSeq
			);

	int recvLen = (len < MAX_BUFFER_LEN - mBuffer.len)?len:(MAX_BUFFER_LEN - mBuffer.len);
	if( recvLen > 0 ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseDataNoCache( "
				"tid : %d, "
				"mBuffer.len : %d, "
				"recvLen : %d "
				")",
				(int)syscall(SYS_gettid),
				mBuffer.len,
				recvLen
				);

		memcpy(mBuffer.buffer + mBuffer.len, buffer, recvLen);
		mBuffer.len += recvLen;
	}

	// 解析命令头
	while(mBuffer.len >= sizeof(CMDH)) {
		// 头部已经收够
		CMDH *header = (CMDH*)mBuffer.buffer;

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseDataNoCache( "
				"tid : %d, "
				"header->scmdt : %d, "
				"header->len : %d, "
				"header->bNew : %s, "
				"header->seq : %d "
				")",
				(int)syscall(SYS_gettid),
				header->cmdt,
				header->len,
				header->bNew?"true":"false",
				header->seq
				);

		int iLen = mBuffer.len - sizeof(CMDH);
		if( iLen >= header->len ) {
			// 参数已经收够
			CMD cmd;
			memcpy(&cmd, mBuffer.buffer, sizeof(CMDH) + header->len);
			cmd.param[header->len] = '\0';

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseDataNoCache( "
					"tid : %d, "
					"cmd->param : \n%s "
					")",
					(int)syscall(SYS_gettid),
					cmd.param
					);

			// 组成协议包成功, 回调
			if( mpClientCallback != NULL ) {
				mpClientCallback->OnParseCmd(this, &cmd);
			}

			// 替换数据
			mBuffer.len -= sizeof(CMDH) + header->len;
			if( mBuffer.len > 0 ) {
				memcpy(mBuffer.buffer, mBuffer.buffer + sizeof(CMDH) + header->len, mBuffer.len);
			}

			ret = 1;

		} else {
			break;
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseData( "
			"tid : %d, "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			ret
			);

	return ret;
}

int Client::AddSeq() {
	int seq;
	mKMutex.lock();
	seq = miPacketSendSeq++;
	mKMutex.unlock();
	return seq;
}
