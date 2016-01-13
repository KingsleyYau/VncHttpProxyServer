/*
 * Session.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "Session.h"

Session::Session(Client* client) {
	// TODO Auto-generated constructor stub
	this->client = client;
}

Session::~Session() {
	// TODO Auto-generated destructor stub
	for(TaskMap::iterator itr = mRequestTaskMap.begin(); itr != mRequestTaskMap.end(); itr++) {
		ITask * task = itr->second;
		if( task != NULL ) {
			delete task;
		}
	}
}

bool Session::InsertRequestTask(int seq, ITask* task) {
	bool bFlag = false;
	TaskMap::iterator itr = mRequestTaskMap.find(seq);
	if( itr == mRequestTaskMap.end() ) {
		mRequestTaskMap.insert(TaskMap::value_type(seq, task));
		bFlag = true;
	}
	return bFlag;
}

ITask* Session::FindRequestTask(int seq) {
	ITask* task = NULL;
	TaskMap::iterator itr = mRequestTaskMap.find(seq);
	if( itr != mRequestTaskMap.end() ) {
		task = itr->second;
	}
	return task;
}

ITask* Session::EraseRequestTask(int seq) {
	ITask* task = NULL;
	TaskMap::iterator itr = mRequestTaskMap.find(seq);
	if( itr != mRequestTaskMap.end() ) {
		task = itr->second;
		mRequestTaskMap.erase(itr);
	}
	return task;
}
