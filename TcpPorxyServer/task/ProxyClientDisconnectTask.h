/*
 * ProxyClientDisconnectTask.h
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#ifndef ProxyClientDisconnectTask_H_
#define ProxyClientDisconnectTask_H_

#include "BaseTask.h"

class ProxyClientDisconnectTask : public BaseTask {
public:
	ProxyClientDisconnectTask();
	virtual ~ProxyClientDisconnectTask();

	void GetSendCmd(CMD* cmd, int seq, int fd);
	bool GetReturnData(const CMD* cmd, char* buffer, int& len);

};

#endif /* ProxyClientDisconnectTask_H_ */
