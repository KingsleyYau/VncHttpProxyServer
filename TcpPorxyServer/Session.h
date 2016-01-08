/*
 * Session.h
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#ifndef SESSION_H_
#define SESSION_H_

#include "task/ITask.h"
#include "Client.h"

#include <map>
using namespace std;

typedef map<int, ITask*> TaskMap;
class Session {
public:
	Session(Client* client);
	virtual ~Session();

	bool InsertRequestTask(int seq, ITask* task);
	ITask* EraseRequestTask(int seq);

	Client* client;

private:
	/**
	 * 外部服务发起的命令列表
	 */
	TaskMap mRequestTaskMap;
};

#endif /* SESSION_H_ */
