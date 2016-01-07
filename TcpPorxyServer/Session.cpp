/*
 * Session.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "Session.h"

Session::Session(Client* client, ITask* task) {
	// TODO Auto-generated constructor stub
	this->client = client;
	this->task = task;
}

Session::~Session() {
	// TODO Auto-generated destructor stub
}
