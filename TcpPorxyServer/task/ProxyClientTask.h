/*
 * ProxyClientTask.h
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#ifndef PROXYCLIENTTASK_H_
#define PROXYCLIENTTASK_H_

#include "BaseTask.h"

class ProxyClientTask : public BaseTask {
public:
	ProxyClientTask();
	virtual ~ProxyClientTask();

	void GetSendCmd(CMD* cmd, int seq);
	bool GetReturnData(const CMD* cmd, char* buffer, int& len);

	void SetBuffer(const char* buffer, int len);

private:
	const char* mpBuffer;
	int miLen;
};

#endif /* PROXYCLIENTTASK_H_ */
