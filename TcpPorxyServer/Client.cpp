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
	mpIdleMessageList = NULL;

	mbError = false;
}

Client::~Client() {
	// TODO Auto-generated destructor stub
	mpIdleMessageList = NULL;
}

void Client::SetClientCallback(ClientCallback* pClientCallback) {
	mpClientCallback = pClientCallback;
}

void Client::SetMessageList(MessageList* pIdleMessageList) {
	mpIdleMessageList = pIdleMessageList;
}

int Client::ParseData(Message* m)  {
	int ret = 0;

	char* buffer = m->buffer;
	int len = m->len;
	int seq = m->seq;

	mKMutex.lock();

	LogManager::GetLogManager()->Log(
			LOG_MSG,
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

	if( mbError ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"Client::ParseData( "
				"tid : %d, "
				"[客户端数据包已经解析错误, 不再处理] "
				")",
				(int)syscall(SYS_gettid)
				);

		ret = -1;
	}

	if( ret != -1 ) {
		if( miDataPacketRecvSeq != seq ) {
			// 收到客户端乱序数据包
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"Client::ParseData( "
					"tid : %d, "
					"[收到客户端乱序的数据包] "
					")",
					(int)syscall(SYS_gettid)
					);

			// 缓存到队列
			Message* mc = mpIdleMessageList->PopFront();
			if( mc != NULL ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"Client::ParseData( "
						"tid : %d, "
						"[缓存客户端乱序的数据包到队列] "
						")",
						(int)syscall(SYS_gettid)
						);

				if( len > 0 ) {
					memcpy(mc->buffer, buffer, len);
				}

				mc->len = len;
				mMessageMap.Insert(seq, mc);

			} else {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"Client::ParseData( "
						"tid : %d, "
						"[客户端没有缓存空间] "
						")",
						(int)syscall(SYS_gettid)
						);

				ret = -1;
			}

		} else {
			// 收到客户端顺序的数据包
			LogManager::GetLogManager()->Log(
					LOG_MSG,
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
					LOG_MSG,
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
						LOG_MSG,
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
								LOG_MSG,
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

						mpIdleMessageList->PushBack(mc);
						mMessageMap.Erase(itr);

						if( ret != -1 ) {
							miDataPacketRecvSeq++;

						} else {
							break;
						}

					} else {
						// 已经没有缓存包
						LogManager::GetLogManager()->Log(
								LOG_MSG,
								"Client::ParseData( "
								"tid : %d, "
								"[已经没有缓存包]"
								")",
								(int)syscall(SYS_gettid)
								);
						break;
					}
				}
			}
		}
	}

	if( ret == -1 ) {
		if( !mbError ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"Client::ParseData( "
					"tid : %d, "
					"[客户端数据包解析错误] "
					")",
					(int)syscall(SYS_gettid)
					);

		}

		mbError = true;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
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
			LOG_MSG,
			"Client::ParseDataNoCache( "
			"tid : %d, "
			"miDataPacketRecvSeq : %d "
			")",
			(int)syscall(SYS_gettid),
			miDataPacketRecvSeq
			);

	int last = len;
	int recved = 0;
	while( ret != -1 && last > 0 ) {
		int recv = (last < MAX_BUFFER_LEN - mBuffer.len)?last:(MAX_BUFFER_LEN - mBuffer.len);
		if( recv > 0 ) {
			memcpy(mBuffer.buffer + mBuffer.len, buffer + recved, recv);
			mBuffer.len += recv;
			last -= recv;
			recved += recv;
		}

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"Client::ParseDataNoCache( "
				"tid : %d, "
				"[接收到数据], "
				"mBuffer.len : %d, "
				"recv : %d, "
				"last : %d, "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				mBuffer.len,
				recv,
				last,
				len
				);

		// 解析命令头
		while(mBuffer.len >= sizeof(CMDH)) {
			// 头部已经收够
			CMDH *header = (CMDH*)mBuffer.buffer;

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"Client::ParseDataNoCache( "
					"tid : %d, "
					"[解析命令头], "
					"header->cmdt : %d, "
					"header->len : %d, "
					"header->bNew : %s, "
					"header->seq : %d, "
					"header->fd : %d "
					")",
					(int)syscall(SYS_gettid),
					header->cmdt,
					header->len,
					header->bNew?"true":"false",
					header->seq,
					header->fd
					);

			if( header->len > MAX_BUFFER_LEN || header->len < 0 ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"Client::ParseDataNoCache( "
						"tid : %d, "
						"[解析命令头错误] "
						")",
						(int)syscall(SYS_gettid)
						);

				ret = -1;
				break;
			}

			int iLen = mBuffer.len - sizeof(CMDH);
			if( iLen >= header->len ) {
				// 参数已经收够
				CMD cmd;
				memcpy(&cmd, mBuffer.buffer, sizeof(CMDH) + header->len);
				cmd.param[header->len] = '\0';

				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"Client::ParseDataNoCache( "
						"tid : %d, "
						"cmd->param : \n%s\n"
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

				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"Client::ParseDataNoCache( "
						"tid : %d, "
						"[替换数据], "
						"mBuffer.len : %d "
						")",
						(int)syscall(SYS_gettid),
						mBuffer.len
						);

				ret = 1;

			} else {
				//
				break;
			}
		}

	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"Client::ParseDataNoCache( "
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
