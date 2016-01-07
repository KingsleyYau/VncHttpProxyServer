/*
 * TcpProxyServer.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef TCPPROXYSERVER_H_
#define TCPPROXYSERVER_H_

#include "Client.h"
#include "Session.h"
#include "MessageList.h"
#include "TcpServer.h"
#include "DataHttpParser.h"

#include <json/json.h>
#include <common/ConfFile.hpp>
#include <common/KSafeMap.h>
#include <common/TimeProc.hpp>
#include <common/StringHandle.h>

#include <map>
#include <list>
using namespace std;

// socket->client
typedef KSafeMap<int, Client*> ClientMap;

// seq->session
typedef KSafeMap<int, Session*> Seq2SessionMap;
// client->session
typedef KSafeMap<int, Session*> Client2SessionMap;

class StateRunnable;
class TcpProxyServer : public TcpServerObserver, ClientCallback {
public:
	TcpProxyServer();
	virtual ~TcpProxyServer();

	void Run(const string& config);
	void Run();
	bool Reload();
	bool IsRunning();

	/**
	 * Implement from TcpServerObserver
	 */
	bool OnAccept(TcpServer *ts, int fd, char* ip);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);
	void OnClose(TcpServer *ts, int fd);
	void OnTimeoutMessage(TcpServer *ts, Message *m);

	/**
	 * 检测状态线程处理
	 */
	void StateRunnableHandle();

	/**
	 * Implement from ClientCallback
	 */
	void OnParseCmd(Client* client, CMD* cmd);

private:
	/*
	 *	请求解析函数
	 *	return : -1:Send fail respond / 0:Continue recv, send no respond / 1:Send OK respond
	 */
	int HandleRecvMessage(TcpServer *ts, Message *m);
	int HandleRecvVNCMessage(TcpServer *ts, Message *m);
	int HandleTimeoutMessage(TcpServer *ts, Message *m);

	/**
	 * 内部服务(HTTP)发起交互请求
	 * @param client	客户端号
	 * @param task		任务
	 */
	bool SendClient2VNC(
			Client* client,
			ITask* task
			);

	/**
	 * 外部服务(VNC)返回交互请求
	 */
	bool ReturnVNC2Client(
			CMD* cmd
			);

	/**
	 * 内部服务(HTTP)关闭会话
	 */
	bool CloseSessionByClient(Client* client);

	/**
	 * 外部服务(VNC)关闭会话
	 */
	bool CloseSessionByVNC();

	/**
	 * 内部服务(HTTP)
	 */
	TcpServer mClientTcpServer;

	/**
	 * 外部服务(VNC)
	 */
	TcpServer mClientTcpVNCServer;

	/**
	 * 配置文件锁
	 */
	KMutex mConfigMutex;

	// BASE
	short miPort;
	int miMaxClient;
	int miMaxHandleThread;
	int miMaxQueryPerThread;
	/**
	 * 请求超时(秒)
	 */
	unsigned int miTimeout;

	// LOG
	int miLogLevel;
	string mLogDir;
	int miDebugMode;

	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * State线程
	 */
	StateRunnable* mpStateRunnable;
	KThread* mpStateThread;

	/**
	 * 统计请求
	 */
	unsigned int mTotal;
	unsigned int mResponed;
	KMutex mCountMutex;

	/**
	 * 配置文件
	 */
	string mConfigFile;

	/**
	 * 监听线程输出间隔
	 */
	unsigned int miStateTime;

	/*
	 * 内部服务(HTTP)在线客户端
	 */
	ClientMap mClientMap;

	/*
	 * 外/内部服务交互会话
	 */
	Seq2SessionMap mSeq2SessionMap;
	Client2SessionMap mClient2SessionMap;

	/**
	 * 外部服务(VNC)
	 */
	Client* mpVNCClient;
};

#endif /* TCPPROXYSERVER_H_ */
