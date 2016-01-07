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
	Session(Client* client, ITask* task);
	virtual ~Session();

	Client* client;
	ITask* task;

};

#endif /* SESSION_H_ */
