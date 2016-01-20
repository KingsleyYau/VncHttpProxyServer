/*
 * TcpProxyClient.cpp
 *
 *  Created on: 2014年3月9日
 *      Author: Kingsley Yau
 *      Email: Kingsleyyau@gmail.com
 */

#define ServerAdess						"192.168.88.140"
#define ServerPort						9874

#include "TcpProxyClient.h"

/*
 * 监听线程
 */
class TcpProxyClientRunnable : public KRunnable {
public:
	TcpProxyClientRunnable(TcpProxyClient *pTcpProxyClient) {
		mpTcpProxyClient = pTcpProxyClient;
	}
	virtual ~TcpProxyClientRunnable() {
		mpTcpProxyClient = NULL;
	}
protected:
	void onRun() {
		if( mpTcpProxyClient != NULL ) {
			mpTcpProxyClient->HandleTcpProxyClientRunnable();
		}
	}
private:
	TcpProxyClient *mpTcpProxyClient;
};

TcpProxyClient::TcpProxyClient() {
	// TODO Auto-generated constructor stub
	mpTcpProxyClientRunnable = new TcpProxyClientRunnable(this);

	mServerAddress = ServerAdess;
	miServerPort = ServerPort;

	mbRunning = false;
}

TcpProxyClient::~TcpProxyClient() {
	// TODO Auto-generated destructor stub
	if( mpTcpProxyClientRunnable ) {
		delete mpTcpProxyClientRunnable;
		mpTcpProxyClientRunnable = NULL;
	}
}

void TcpProxyClient::SetTcpProxyClientCallback(TcpProxyClientCallback *pTcpProxyClientCallback) {
	mpTcpProxyClientCallback = pTcpProxyClientCallback;
}

bool TcpProxyClient::Start() {
	bool bFlag = false;

	if( !mbRunning ) {
		mbRunning = true;

		if( -1 != mKThread.start(mpTcpProxyClientRunnable) ) {
			bFlag = true;
		} else {
			mbRunning = false;
		}
	}
	return bFlag;
}

void TcpProxyClient::Stop() {
	mbRunning = false;
	mTcpSocket.Close();
	mKThread.stop();
}

bool TcpProxyClient::ConnectServer() {
	bool bFlag = false;

	int iRet = mTcpSocket.Connect(mServerAddress.c_str(), miServerPort, true);
	if(iRet > 0) {
		bFlag = true;

		mSendMutex.lock();
		mSendSeq = 0;
		mSendMutex.unlock();
	}

	return bFlag;
}

void TcpProxyClient::HandleTcpProxyClientRunnable() {
	CMD cmd;
	int seq = 0;

	while( mbRunning ) {
		printf("# TcpProxyClient::HandleTcpProxyClientRunnable( 等待连接服务端 ) \n");
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyClient::HandleTcpProxyClientRunnable( "
				"[等待连接服务端] "
				")"
				);

		if( ConnectServer() ) {
			if( mpTcpProxyClientCallback != NULL ) {
				mpTcpProxyClientCallback->OnConnected(this);
			}

			printf("# TcpProxyClient::HandleTcpProxyClientRunnable( 已经连接上服务端 ) \n");
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyClient::HandleTcpProxyClientRunnable( "
					"[已经连接上服务端] "
					")"
					);

			// 连接上服务端
			bool bCanRecv = true;
			while(bCanRecv) {
				// 开始接收命令
//				printf("# TcpProxyClient::HandleTcpProxyClientRunnable( 等待服务端命令 ) \n");
//				LogManager::GetLogManager()->Log(
//						LOG_MSG,
//						"TcpProxyClient::HandleTcpProxyClientRunnable( "
//						"[等待服务端命令] "
//						")"
//						);

				CMD cmd;
				bCanRecv = RecvCommand(cmd);
				if( bCanRecv ) {
					OnRecvCommand(cmd);

				} else {
					// 与服务端连接已经断开
					printf("# TcpProxyClient::HandleTcpProxyClientRunnable( 与服务端连接已经断开 ) \n");
					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"TcpProxyClient::HandleTcpProxyClientRunnable( "
							"[与服务端连接已经断开] "
							")"
							);

					if( mpTcpProxyClientCallback != NULL ) {
						mpTcpProxyClientCallback->OnDisConnected(this);
					}

					mTcpSocket.Close();

					break;
				}
			}
		}
		sleep(10);
	}
}

bool TcpProxyClient::RecvCommand(CMD &cmd) {
	bool bFlag = false;
	bzero(&cmd, sizeof(CMD));
	cmd.header.cmdt = CommandTypeNone;

	// 接收命令头
	char HeadBuffer[sizeof(CMDH) + 1] = {'\0'};
	int len = mTcpSocket.RecvData(HeadBuffer, sizeof(CMDH));
	if(len == sizeof(CMDH)) {
		// 接收头成功
		memcpy(&cmd.header, HeadBuffer, sizeof(CMDH));

		if( cmd.header.len > 0 ) {
			len = mTcpSocket.RecvData(cmd.param, cmd.header.len);
			if( len == cmd.header.len ) {
				// 接收参数成功
				cmd.param[cmd.header.len] = '\0';
				bFlag = true;
			}

		} else {
			// 没有参数
			bFlag = true;
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpProxyClient::RecvCommand( "
			"cmd.header.cmdt : %d, "
			"cmd.header.seq : %d, "
			"cmd.header.fd : %d, "
			"cmd.header.bNew : %s, "
			"cmd.header.len : %d "
			")",
			cmd.header.cmdt,
			cmd.header.seq,
			cmd.header.fd,
			cmd.header.bNew?"true":"false",
			cmd.header.len
			);

	return bFlag;
}

void TcpProxyClient::OnRecvCommand(const CMD &cmd) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyClient::OnRecvCommand( "
			"[收到服务端命令], "
			"cmd.header.cmdt : %d, "
			"cmd.header.seq : %d, "
			"cmd.header.fd : %d, "
			"cmd.header.bNew : %s, "
			"cmd.header.len : %d, "
			"cmd.param : \n%s\n"
			")",
			cmd.header.cmdt,
			cmd.header.seq,
			cmd.header.fd,
			cmd.header.bNew?"true":"false",
			cmd.header.len,
			cmd.param
			);

	switch(cmd.header.cmdt) {
	case CommandTypeProxy:{
		// 解析代理Http请求
		HandleRecvProxyBuffer(cmd);

	}break;
	case CommandTypeProxyDisconnect:{
		// 解析代理断开连接
		HandleRecvProxyDisconnect(cmd);

	}break;
	case CommandTypeNone:{
		// 与服务端连接已经断开

	}break;
	default:break;
	}
}

void TcpProxyClient::HandleRecvProxyBuffer(const CMD &cmd) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyClient::OnRecvProxyDisconnect( "
			"[收到命令:代理Http请求], "
			"cmd.header.fd : %d, "
			"cmd.header.seq : %d "
			")",
			cmd.header.fd,
			cmd.header.seq
			);
	if( mpTcpProxyClientCallback != NULL ) {
		mpTcpProxyClientCallback->OnRecvProxyBuffer(this, cmd.header.seq, cmd.header.fd, cmd.param, cmd.header.len);
	}
}

void TcpProxyClient::HandleRecvProxyDisconnect(const CMD &cmd) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyClient::OnRecvProxyDisconnect( "
			"[收到命令:代理断开请求], "
			"cmd.header.fd : %d, "
			"cmd.header.seq : %d "
			")",
			cmd.header.fd,
			cmd.header.seq
			);
	if( mpTcpProxyClientCallback != NULL ) {
		mpTcpProxyClientCallback->OnRecvProxyDisconnect(this, cmd.header.seq, cmd.header.fd);
	}
}

bool TcpProxyClient::SendProxyBuffer(int seq, int fd, const char* buffer, int len) {
	CMD cmd;
	cmd.header.cmdt = CommandTypeProxy;
	cmd.header.bNew = false;
	cmd.header.seq = seq;
	cmd.header.fd = fd;

	bool bFlag = true;
	int sent = 0;
	int send = 0;
	int last = len;
	while( last > 0 && bFlag ) {
		send = MIN(last, MAX_PARAM_LEN - 1);

		cmd.header.len = send;
		memcpy(cmd.param, buffer + sent, cmd.header.len);
		cmd.param[cmd.header.len] = '\0';

		sent += send;
		last -= send;

		bFlag = SendCommand(cmd);
	}

	return bFlag;
}

/**
 * 返回代理断开连接到服务器
 */
bool TcpProxyClient::SendProxyDisconnect(int fd) {
	CMD cmd;
	cmd.header.cmdt = CommandTypeProxyDisconnect;
	cmd.header.bNew = true;
	mSendMutex.lock();
	cmd.header.seq = mSendSeq++;
	mSendMutex.unlock();
	cmd.header.fd = fd;
	cmd.header.len = 0;
	cmd.param[cmd.header.len] = '\0';
	return SendCommand(cmd);
}

bool TcpProxyClient::SendCommand(const CMD &cmd) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpProxyClient::SendCommand( "
			"cmd.header.cmdt : %d, "
			"cmd.header.seq : %d, "
			"cmd.header.fd : %d, "
			"cmd.header.bNew : %s, "
			"cmd.header.len : %d, "
			"cmd.param : \n%s\n"
			")",
			cmd.header.cmdt,
			cmd.header.seq,
			cmd.header.fd,
			cmd.header.bNew?"true":"false",
			cmd.header.len,
			cmd.param
			);

	// 发送命令
	int iLen = sizeof(CMDH) + cmd.header.len;
	int iSend = mTcpSocket.SendData((char *)&cmd, iLen);
	if( iSend == iLen ) {
		bFlag = true;
	}

	return bFlag;
}
