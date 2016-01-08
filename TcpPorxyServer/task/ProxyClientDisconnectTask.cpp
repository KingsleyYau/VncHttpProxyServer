/*
 * ProxyClientDisconnectTask.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "ProxyClientDisconnectTask.h"

ProxyClientDisconnectTask::ProxyClientDisconnectTask() {
	// TODO Auto-generated constructor stub
}

ProxyClientDisconnectTask::~ProxyClientDisconnectTask() {
	// TODO Auto-generated destructor stub
}

void ProxyClientDisconnectTask::GetSendCmd(CMD* cmd, int seq, int fd) {
	this->seq = seq;

	cmd->header.cmdt = CommandTypeProxyDisconnect;
	cmd->header.bNew = true;
	cmd->header.seq = seq;
	cmd->header.fd = fd;
	cmd->header.len = 0;

}

bool ProxyClientDisconnectTask::GetReturnData(const CMD* cmd, char* buffer, int& len) {
	bool bFlag = true;
	return bFlag;
}
