/*
 * TcpProxyServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "TcpProxyServer.h"

#include "task/ProxyClientTask.h"
#include "task/ProxyClientDisconnectTask.h"

#include <sys/syscall.h>

/* state thread */
class StateRunnable : public KRunnable {
public:
	StateRunnable(TcpProxyServer *container) {
		mContainer = container;
	}
	virtual ~StateRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->StateRunnableHandle();
	}
private:
	TcpProxyServer *mContainer;
};

TcpProxyServer::TcpProxyServer() {
	// TODO Auto-generated constructor stub
	mpStateRunnable = new StateRunnable(this);
	mpStateThread = NULL;

	miPort = 0;
	miMaxClient = 0;
	miMaxHandleThread = 0;
	miMaxQueryPerThread = 0;
	miTimeout = 0;

	miStateTime = 0;
	miDebugMode = 0;
	miLogLevel = 0;

	mResponed = 0;
	mTotal = 0;

	mIsRunning = false;

	mpVNCClient = NULL;
}

TcpProxyServer::~TcpProxyServer() {
	// TODO Auto-generated destructor stub
	if( mpStateRunnable ) {
		delete mpStateRunnable;
	}
}

void TcpProxyServer::Run(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// Reload config
		if( Reload() ) {
			if( miDebugMode == 1 ) {
				LogManager::LogSetFlushBuffer(0);
			} else {
				LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
			}

			Run();
		} else {
			printf("# Tcp Proxy Server can not load config file exit. \n");
		}

	} else {
		printf("# No config file can be use exit. \n");
	}
}

void TcpProxyServer::Run() {
	/* log system */
	LogManager::GetLogManager()->Start(1000, miLogLevel, mLogDir);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::Run( "
			"miPort : %d, "
			"miMaxClient : %d, "
			"miMaxHandleThread : %d, "
			"miMaxQueryPerThread : %d, "
			"miTimeout : %d, "
			"miStateTime, %d, "
			"miLogLevel : %d, "
			"mlogDir : %s "
			")",
			miPort,
			miMaxClient,
			miMaxHandleThread,
			miMaxQueryPerThread,
			miTimeout,
			miStateTime,
			miLogLevel,
			mLogDir.c_str()
			);

	/**
	 * 客户端缓存buffer
	 */
	for(int i = 0; i < miMaxClient; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}

	/* client server */
	/**
	 * 预估相应时间, 内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetTcpServerObserver(this);
	mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);
	mClientTcpServer.Start(miMaxClient, miPort, miMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpProxyServer::Run( TcpServer Init OK )");

	/* vnc server */
	mClientTcpVNCServer.SetTcpServerObserver(this);
	/**
	 * 预估相应时间, 内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpVNCServer.SetHandleSize(miTimeout * miMaxQueryPerThread);
	mClientTcpVNCServer.Start(2, miPort + 1, miMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpProxyServer::Run( Inside TcpServer Init OK )");

	mIsRunning = true;

	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != 0 ) {
	}

	printf("# TcpProxyServer start OK. \n");
	LogManager::GetLogManager()->Log(LOG_WARNING, "TcpProxyServer::Run( Init OK )");

	/* call server */
	while( true ) {
		/* do nothing here */
		sleep(5);
	}
}

bool TcpProxyServer::Reload() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// BASE
			miPort = atoi(conf.GetPrivate("BASE", "PORT", "9876").c_str());
			miMaxClient = atoi(conf.GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
			miMaxHandleThread = atoi(conf.GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
			miMaxQueryPerThread = atoi(conf.GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());
			miTimeout = atoi(conf.GetPrivate("BASE", "TIMEOUT", "10").c_str());
			miStateTime = atoi(conf.GetPrivate("BASE", "STATETIME", "30").c_str());

			// LOG
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			mClientTcpServer.SetHandleSize(miTimeout * miMaxQueryPerThread);

			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::Reload( "
					"miPort : %d, "
					"miMaxClient : %d, "
					"miMaxHandleThread : %d, "
					"miMaxQueryPerThread : %d, "
					"miTimeout : %d, "
					"miStateTime, %d, "
					"miLogLevel : %d, "
					"mlogDir : %s "
					")",
					miPort,
					miMaxClient,
					miMaxHandleThread,
					miMaxQueryPerThread,
					miTimeout,
					miStateTime,
					miLogLevel,
					mLogDir.c_str()
					);

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool TcpProxyServer::IsRunning() {
	return mIsRunning;
}

/**
 * New request
 */
bool TcpProxyServer::OnAccept(TcpServer *ts, int fd, char* ip) {
	if( ts == &mClientTcpServer ) {
		Client *client = new Client();
		client->SetClientCallback(this);
		client->SetMessageList(&mIdleMessageList);
		client->fd = fd;
		client->ip = ip;
		client->isOnline = true;

		mClientMap.Lock();
		mClientMap.Insert(fd, client);
		mClientMap.Unlock();

		mCountMutex.lock();
		mTotal++;
		mCountMutex.unlock();

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyServer::OnAccept( "
				"tid : %d, "
				"fd : [%d], "
				"[内部服务(HTTP), 建立连接] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

	} else if( ts == &mClientTcpVNCServer ) {
		if( mpVNCClient == NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnAccept( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 建立连接] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

			mpVNCClient = new Client();
			mpVNCClient->SetClientCallback(this);
			mpVNCClient->SetMessageList(&mIdleMessageList);
			mpVNCClient->fd = fd;
			mpVNCClient->ip = ip;
			mpVNCClient->isOnline = true;

		} else {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnAccept( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 已经建立连接, 不需要重新建立] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

			return false;
		}

	}

	return true;
}

void TcpProxyServer::OnDisconnect(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyServer::OnDisconnect( "
				"tid : %d, "
				"fd : [%d], "
				"[内部服务(HTTP), 断开连接] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

		// 标记下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		Client* client = itr->second;
		if( client != NULL ) {
			client->isOnline = false;

			// 断开代理命令
			ProxyClientDisconnectTask* task = new ProxyClientDisconnectTask();
			SendClient2VNC(client, (ITask*)task, false);
			delete task;

		}
		mClientMap.Unlock();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"TcpProxyServer::OnDisconnect( "
				"tid : %d, "
				"fd : [%d], "
				"[内部服务(HTTP), 断开连接完成] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

	} else if( ts == &mClientTcpVNCServer ) {
		if( mpVNCClient != NULL && mpVNCClient->fd == fd ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnDisconnect( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 断开连接] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

			mpVNCClient->isOnline = false;

		}

	}

}

void TcpProxyServer::OnClose(TcpServer *ts, int fd) {
	if( ts == &mClientTcpServer ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyServer::OnClose( "
				"tid : %d, "
				"fd : [%d], "
				"[内部服务(HTTP), 关闭socket] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

		// 释放资源下线
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(fd);
		if( itr != mClientMap.End() ) {
			Client* client = itr->second;
			CloseSessionByClient(client);
			delete client;

			mClientMap.Erase(itr);
		}
		mClientMap.Unlock();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"TcpProxyServer::OnClose( "
				"tid : %d, "
				"fd : [%d], "
				"[内部服务(HTTP), 关闭socket完成] "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

	} else if( ts == &mClientTcpVNCServer ) {
		if( mpVNCClient != NULL && mpVNCClient->fd == fd ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnClose( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 关闭socket] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

			CloseSessionByVNC();
			delete mpVNCClient;
			mpVNCClient = NULL;

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpProxyServer::OnClose( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 关闭socket完成] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

		} else {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnClose( "
					"tid : %d, "
					"fd : [%d], "
					"[外部服务(VNC), 关闭非法socket] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
		}

	}

}

void TcpProxyServer::OnRecvMessage(TcpServer *ts, Message *m) {
	if( &mClientTcpServer == ts ) {
		// 内部服务(HTTP)请求
		HandleRecvMessage(ts, m);

	} else if( &mClientTcpVNCServer == ts ){
		HandleRecvVNCMessage(ts, m);

	}

}

void TcpProxyServer::OnSendMessage(TcpServer *ts, Message *m) {

}

void TcpProxyServer::OnTimeoutMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::OnTimeoutMessage( "
			"tid : %d, "
			"m->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	HandleTimeoutMessage(ts, m);

}

void TcpProxyServer::StateRunnableHandle() {
	unsigned int iCount = 0;
	unsigned int iStateTime = miStateTime;

	unsigned int iTotal = 0;
	double iSecondTotal = 0;
	double iResponed = 0;

	while( IsRunning() ) {
		if ( iCount < iStateTime ) {
			iCount++;

		} else {
			iCount = 0;
			iSecondTotal = 0;
			iResponed = 0;

			mCountMutex.lock();
			iTotal = mTotal;

			if( iStateTime != 0 ) {
				iSecondTotal = 1.0 * iTotal / iStateTime;
			}
			if( iTotal != 0 ) {
				iResponed = 1.0 * mResponed / iTotal;
			}

			mTotal = 0;
			mResponed = 0;
			mCountMutex.unlock();

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"TcpProxyServer::StateRunnable( "
					"tid : %d, "
					"过去%u秒共收到%u个请求, "
					"平均收到%.1lf个/秒, "
					"平均响应时间%.1lf毫秒/个"
					")",
					(int)syscall(SYS_gettid),
					iStateTime,
					iTotal,
					iSecondTotal,
					iResponed
					);

			iStateTime = miStateTime;

		}
		sleep(1);
	}
}

bool TcpProxyServer::SendClient2VNC(
		Client* client,
		ITask* task,
		bool needReturn
		) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::SendClient2VNC( "
			"tid : %d, "
			"[外部服务(VNC), 发送命令到VNC], "
			"client->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);

	bool bFlag = false;

	if( mpVNCClient == NULL || !(mpVNCClient->isOnline) ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyServer::SendClient2VNC( "
				"tid : %d, "
				"[外部服务(VNC), 发送命令到VNC, 服务不在线], "
				"client->fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				client->fd
				);

		return bFlag;
	}

	Message* sm = mClientTcpVNCServer.GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		int seq = mpVNCClient->AddSeq();

		Session* session = NULL;
		mClient2SessionMap.Lock();
		Client2SessionMap::iterator itr = mClient2SessionMap.Find(client->fd);
		if( itr != mClient2SessionMap.End() ) {
			session = itr->second;
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::SendClient2VNC( "
					"tid : %d, "
					"[外部服务(VNC), 发送命令到VNC, 继续会话], "
					"client->fd : [%d], "
					"seq : %d "
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					seq
					);

		} else {
			session = new Session(client);
			mClient2SessionMap.Insert(client->fd, session);
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::SendClient2VNC( "
					"tid : %d, "
					"[外部服务(VNC), 发送命令到VNC, 开始新会话], "
					"client->fd : [%d], "
					"seq : %d "
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					seq
					);
		}

		if( needReturn ) {
			session->InsertRequestTask(seq, task);
		}

		CMD* cmd = (CMD*)sm->buffer;
		task->GetSendCmd(cmd, seq, client->fd);

		sm->fd = mpVNCClient->fd;
		sm->len = sizeof(CMDH) + cmd->header.len;

		mClientTcpVNCServer.SendMessageByQueue(sm);

		bFlag = true;

		mClient2SessionMap.Unlock();
	}

	return bFlag;
}

bool TcpProxyServer::ReturnVNC2Client(
		CMD* cmd
		) {
	bool bFlag = false;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpProxyServer::ReturnVNC2Client( "
			"tid : %d, "
			"cmd->header.seq : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid),
			cmd->header.seq
			);

	mClient2SessionMap.Lock();
	Client2SessionMap::iterator itr = mClient2SessionMap.Find(cmd->header.fd);
	if( itr != mClient2SessionMap.End() ) {
		// 内部服务(HTTP), 客户端在会话中
		Session* session = itr->second;
		if( session != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::ReturnVNC2Client( "
					"tid : %d, "
					"[内部服务(HTTP), 返回数据到客户端, 客户端在会话中], "
					"cmd->header.fd : [%d], "
					"cmd->header.seq : %d "
					")",
					(int)syscall(SYS_gettid),
					cmd->header.fd,
					cmd->header.seq
					);

			ITask* task = session->FindRequestTask(cmd->header.seq);
			if( task != NULL ) {
				// 会话中存在对应的命令号
//				LogManager::GetLogManager()->Log(
//						LOG_MSG,
//						"TcpProxyServer::ReturnVNC2Client( "
//						"tid : %d, "
//						"[内部服务(HTTP), 返回数据到客户端, 会话中存在对应的命令号], "
//						"session->client->fd : [%d], "
//						"task : %p "
//						")",
//						(int)syscall(SYS_gettid),
//						session->client->fd,
//						task
//						);

				Message* sm = mClientTcpServer.GetIdleMessageList()->PopFront();
				if( sm != NULL ) {
					sm->fd = session->client->fd;
					task->GetReturnData(cmd, sm->buffer, sm->len);

					LogManager::GetLogManager()->Log(
							LOG_MSG,
							"TcpProxyServer::ReturnVNC2Client( "
							"tid : %d, "
							"[内部服务(HTTP), 返回数据到客户端, 会话中存在对应的命令号], "
							"session->client->fd : [%d], "
							"buffer( len : %d ) : \n%s\n"
							")",
							(int)syscall(SYS_gettid),
							session->client->fd,
							sm->len,
							sm->buffer
							);

					mClientTcpServer.SendMessageByQueue(sm);

					bFlag = true;
				}

//				delete task;
//				task = NULL;

			} else {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"TcpProxyServer::ReturnVNC2Client( "
						"tid : %d, "
						"session->client->fd : [%d], "
						"[内部服务(HTTP), 返回数据到客户端, 会话中不存在对应命令号] "
						")",
						(int)syscall(SYS_gettid),
						session->client->fd
						);
			}

		} else {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::ReturnVNC2Client( "
					"tid : %d, "
					"[内部服务(HTTP), 返回数据到客户端, 客户端不在会话中], "
					"cmd->header.fd : [%d], "
					"cmd->header.seq : %d "
					")",
					(int)syscall(SYS_gettid),
					cmd->header.fd,
					cmd->header.seq
					);
		}

	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"TcpProxyServer::ReturnVNC2Client( "
				"tid : %d, "
				"cmd->header.seq : %d, "
				"[内部服务(HTTP), 返回数据到客户端, 客户端不在会话中] "
				")",
				(int)syscall(SYS_gettid),
				cmd->header.seq
				);
	}
	mClient2SessionMap.Unlock();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpProxyServer::ReturnVNC2Client( "
			"tid : %d, "
			"cmd->header.seq : %d, "
			"end "
			")",
			(int)syscall(SYS_gettid),
			cmd->header.seq
			);

	return bFlag;
}

bool TcpProxyServer::CloseSessionByVNC() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::CloseSessionByVNC( "
			"tid : %d, "
			"[外部服务(VNC), 关闭所有会话] "
			")",
			(int)syscall(SYS_gettid)
			);

	bool bFlag = false;

	// 断开所有客户端连接
	mClientMap.Lock();
	for(ClientMap::iterator itr = mClientMap.Begin(); itr != mClientMap.End(); itr++) {
		Client* client = itr->second;
		if( client != NULL ) {
			mClientTcpServer.Disconnect(client->fd);
		}
	}
	mClientMap.Unlock();

	return bFlag;
}

bool TcpProxyServer::CloseSessionByClient(Client* client) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::CloseSessionByClient( "
			"tid : %d, "
			"[内部服务(HTTP), 关闭会话], "
			"client->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);
	bool bFlag = false;

	mClient2SessionMap.Lock();
	Client2SessionMap::iterator itr = mClient2SessionMap.Find(client->fd);
	if( itr != mClient2SessionMap.End() ) {
		Session* session = itr->second;
		if( session != NULL ) {
			delete session;
			session = NULL;

		}

		mClient2SessionMap.Erase(itr);

		bFlag = true;
	}
	mClient2SessionMap.Unlock();

	return bFlag;
}


void TcpProxyServer::OnParseCmd(Client* client, CMD* cmd) {
	bool bFlag = false;

	if( client != NULL && cmd != NULL ) {
		if( client == mpVNCClient ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::OnParseCmd( "
					"tid : %d, "
					"[外部服务(VNC), 返回命令], "
					"fd : [%d], "
					"cmd->header.cmdt : %d, "
					"cmd->param( len : %d ) "
					")",
					(int)syscall(SYS_gettid),
					cmd->header.fd,
					cmd->header.cmdt,
					cmd->header.len
					);

			switch(cmd->header.cmdt) {
			case CommandTypeProxyDisconnect:{
				if( cmd->header.bNew ) {
					// 断开客户端连接
					mClientTcpServer.Disconnect(cmd->header.fd);
				}

			}break;
			default:{
				bFlag = ReturnVNC2Client(cmd);

			}break;
			}

		}
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpProxyServer::OnParseCmd( "
			"tid : %d, "
			"bFlag : %s "
			")",
			(int)syscall(SYS_gettid),
			bFlag?"true":"false"
			);
}

int TcpProxyServer::HandleRecvMessage(TcpServer *ts, Message *m) {
	int ret = -1;

	if( m->buffer != NULL ) {
		Client *client = NULL;

		/**
		 * 因为还有数据包处理队列中, TcpServer不会回调OnClose, 所以不怕client被释放
		 * 放开锁就可以使多个client并发解数据包, 单个client解包只能同步解包, 在client内部加锁
		 */
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(m->fd);
		if( itr != mClientMap.End() ) {
			client = itr->second;
			mClientMap.Unlock();

		} else {
			mClientMap.Unlock();

		}

		if( client != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"TcpProxyServer::HandleRecvMessage( "
					"tid : %d, "
					"[内部服务(HTTP), 客户端发起请求], "
					"fd : [%d], "
					"buffer : [\n%s\n]"
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					m->buffer
					);

			// 转发代理命令
			ProxyClientTask* task = new ProxyClientTask();
			task->SetBuffer(m->buffer, m->len);

			// 发送命令
			if( SendClient2VNC(client, (ITask*)task) ) {
				// 发送成功
				ret = 0;

			} else {
				// 发送失败
				delete task;

			}
		}

	}

	if( ret != 0 ) {
		ts->Disconnect(m->fd);
	}

	return ret;
}

int TcpProxyServer::HandleRecvVNCMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpProxyServer::HandleRecvVNCMessage( "
			"tid : %d, "
			"m->fd: [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	int ret = 0;

	if( m->buffer != NULL ) {
		ret = mpVNCClient->ParseData(m);
		if( ret == -1 ) {
			ts->Disconnect(m->fd);

		}

	}

	return ret;
}

int TcpProxyServer::HandleTimeoutMessage(TcpServer *ts, Message *m) {
	int ret = -1;

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"TcpProxyServer::HandleTimeoutMessage( "
			"tid : %d, "
			"[请求超时]"
			"m->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	// 断开连接
	ts->Disconnect(m->fd);

	return ret;
}
