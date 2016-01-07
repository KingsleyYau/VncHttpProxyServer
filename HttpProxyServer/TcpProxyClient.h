/*
 * SnifferClient.h
 *
 *  Created on: 2014年3月9日
 *      Author: Kingsley Yau
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef SNIFFERCLIENT_H_
#define SNIFFERCLIENT_H_

#include <CommandDef.h>

#include <common/KTcpSocket.h>
#include <common/KThread.h>
//#include <common/IPAddress.h>
//#include <common/command.h>
#include <common/md5.h>
#include <common/StringHandle.h>

class TcpProxyClient;
class TcpProxyClientCallback {
public:
	virtual ~TcpProxyClientCallback(){};
	virtual void OnConnected(TcpProxyClient* client) = 0;
	virtual void OnDisConnected(TcpProxyClient* client) = 0;

	virtual void OnRecvProxyBuffer(TcpProxyClient* client, int seq, int fd, const char* buffer, int len) = 0;

};
class TcpProxyClientRunnable;
class TcpProxyClient {
public:
	TcpProxyClient();
	virtual ~TcpProxyClient();

	void SetTcpProxyClientCallback(TcpProxyClientCallback *pCallback);
	bool Start();
	void Stop();

	/**
	 * 返回代理Http请求结果到服务器
	 */
	bool SendProxyBuffer(bool bFlag, int seq, int fd, const char* buffer, int len);

	// 处理线程
	void HandleTcpProxyClientRunnable();

private:
	/*
	 * 连接服务器
	 */
	bool ConnectServer();

	/*
	 * 发送命令到服务器
	 */
	bool SendCommand(const CMD &cmd);

	/*
	 * 接收服务器命令
	 */
	bool RecvCommand(CMD &cmd);

	/*
	 * 分发接收命令
	 */
	void OnRecvCommand(const CMD &cmd);

	/*###################### 解析接收命令  ######################*/
	/**
	 * 解析代理Http请求
	 */
	void HandleRecvProxyBuffer(const CMD &cmd);


	TcpProxyClientCallback* mpTcpProxyClientCallback;
	bool mbRunning;

	KThread mKThread;
	TcpProxyClientRunnable* mpTcpProxyClientRunnable;

	KTcpSocket mTcpSocket;
	string mServerAddress;
	int miServerPort;

	KMutex mSendMutex;
	int mSendSeq;

};

#endif /* SNIFFERCLIENT_H_ */
