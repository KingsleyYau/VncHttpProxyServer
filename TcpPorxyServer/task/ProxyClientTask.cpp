/*
 * ProxyClientTask.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "ProxyClientTask.h"

ProxyClientTask::ProxyClientTask() {
	// TODO Auto-generated constructor stub
	mpBuffer = NULL;
	miLen = 0;
}

ProxyClientTask::~ProxyClientTask() {
	// TODO Auto-generated destructor stub
}

void ProxyClientTask::GetSendCmd(CMD* cmd, int seq) {
	this->seq = seq;

	cmd->header.cmdt = CommandTypeProxy;
	cmd->header.bNew = true;
	cmd->header.seq = seq;
	cmd->header.len = miLen;

	if( mpBuffer != NULL && miLen > 0 ) {
		memcpy(cmd->param, mpBuffer, cmd->header.len);
	}
}

bool ProxyClientTask::GetReturnData(const CMD* cmd, char* buffer, int& len) {
	bool bFlag = false;

	if( buffer != NULL ) {
		if( cmd->header.len > 0 && cmd->param != NULL ) {
			len = cmd->header.len;
			memcpy(buffer, cmd->param, len);
		}
	}

	return bFlag;
}

void ProxyClientTask::SetBuffer(const char* buffer, int len) {
	mpBuffer = buffer;
	miLen = len;
}
