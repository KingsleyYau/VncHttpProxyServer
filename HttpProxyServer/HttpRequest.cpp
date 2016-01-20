/*
 * HttpRequest.cpp
 *
 *  Created on: 2015-2-27
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "HttpRequest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

class HttpRequestRunnable : public KRunnable {
public:
	HttpRequestRunnable(HttpRequest* pHttpRequest) {
		mpHttpRequest = pHttpRequest;
	}
	~HttpRequestRunnable(){
		mpHttpRequest = NULL;
	};

protected:
	void onRun() {
		bool bFlag = mpHttpRequest->mHttpClient.Request(mpHttpRequest->mUrl, mpHttpRequest->mHeaders);
		if( bFlag ) {
			if( mpHttpRequest->mpIHttpRequestCallback != NULL ) {
				mpHttpRequest->mpIHttpRequestCallback->onSuccess(
						mpHttpRequest,
						mpHttpRequest->mpRespondBuffer,
						mpHttpRequest->miCurrentSize);
			}
		} else {
			if( mpHttpRequest->mpIHttpRequestCallback != NULL ) {
				mpHttpRequest->mpIHttpRequestCallback->onFail(
						mpHttpRequest
						);
			}
		}
	}

private:
	HttpRequest *mpHttpRequest;
};

HttpRequest::HttpRequest() {
	// TODO Auto-generated constructor stub
	mpHttpRequestRunnable = new HttpRequestRunnable(this);
	mHttpClient.SetCallback(this);
	mpRespondBuffer = NULL;
	mpRequestBuffer = NULL;
	mbCache = false;
	miCurrentSize = 0;
	mpIHttpRequestCallback = NULL;

	fd = -1;
	seq = -1;
}

HttpRequest::~HttpRequest() {
	// TODO Auto-generated destructor stub
	mHttpClient.Stop();

	if( mpHttpRequestRunnable != NULL ) {
		delete mpHttpRequestRunnable;
		mpHttpRequestRunnable = NULL;
	}

	if( mpRespondBuffer != NULL ) {
		delete[] mpRespondBuffer;
		mpRespondBuffer = NULL;
	}

	if( mpRequestBuffer != NULL ) {
		delete[] mpRequestBuffer;
		mpRequestBuffer = NULL;
	}
}

long HttpRequest::StartRequest(const string& url, const list<string> headers) {
	InitRespondBuffer();

	mUrl = url;
	std::copy(headers.begin(), headers.end(), std::back_inserter(mHeaders));

	mKThread.stop();
	return mKThread.start(mpHttpRequestRunnable);
}

void HttpRequest::StopRequest(bool bWait) {
	mHttpClient.Stop();
	if( bWait ) {
		mKThread.stop();
	}
}
void HttpRequest::SetCallback(IHttpRequestCallback *callback){
	mpIHttpRequestCallback = callback;
}

// 设置是否缓存返回结果, 默认是缓存
void HttpRequest::SetNoCacheData(bool bCache) {
	mbCache = bCache;
}

void HttpRequest::SetSendData(const char* buffer, int len) {
	InitRequestBuffer();
	AddRequestBuffer(buffer, len);
}

void HttpRequest::SetFd(int fd, int seq) {
	this->fd = fd;
	this->seq = seq;
}

int HttpRequest::Getfd() {
	return fd;
}

int HttpRequest::GetSeq() {
	return seq;
}

void HttpRequest::onReceiveBody(HttpClient* client, const char* buf, int size) {
	// 如果不缓存, 成功返回数据为0
	if( !mbCache ) {
		AddRespondBuffer(buf, size);
	}

	if( mpIHttpRequestCallback != NULL ) {
		mpIHttpRequestCallback->onReceiveBody(
				this,
				buf,
				size
				);
	}
}

int HttpRequest::onSendBody(HttpClient* client, char* buf, int size) {
	int sendSize = MIN(miRequestCurrentSize - miRequestCurrentSend, size);
	if( sendSize > 0 ) {
		memcpy(buf, mpRequestBuffer + miRequestCurrentSend, sendSize);
		miRequestCurrentSend += sendSize;
	}
	return sendSize;
}

void HttpRequest::InitRespondBuffer() {
	if( mpRespondBuffer != NULL ) {
		delete[] mpRespondBuffer;
		mpRespondBuffer = NULL;
	}

	miCurrentSize = 0;
	miCurrentCapacity = MAX_RESPONED_BUFFER;
	mpRespondBuffer = new char[miCurrentCapacity];
}

bool HttpRequest::AddRespondBuffer(const char* buf, int size) {
	bool bFlag = false;
	/* Add buffer if buffer is not enough */
	while( size + miCurrentSize >= miCurrentCapacity ) {
		miCurrentCapacity *= 2;
		bFlag = true;
	}
	if( bFlag ) {
		char *newBuffer = new char[miCurrentCapacity];
		if( mpRespondBuffer != NULL ) {
			memcpy(newBuffer, mpRespondBuffer, miCurrentSize);
			delete[] mpRespondBuffer;
			mpRespondBuffer = NULL;
		}
		mpRespondBuffer = newBuffer;
	}
	memcpy(mpRespondBuffer + miCurrentSize, buf, size);
	miCurrentSize += size;
	mpRespondBuffer[miCurrentSize] = '\0';
	return true;
}

void HttpRequest::InitRequestBuffer() {
	if( mpRequestBuffer != NULL ) {
		delete[] mpRequestBuffer;
		mpRequestBuffer = NULL;
	}

	miRequestCurrentSend = 0;
	miRequestCurrentSize = 0;
	miRequestCurrentCapacity = MAX_RESPONED_BUFFER;
	mpRequestBuffer = new char[miRequestCurrentCapacity];
}

bool HttpRequest::AddRequestBuffer(const char* buf, int size) {
	bool bFlag = false;
	if( size > 0 ) {
		/* Add buffer if buffer is not enough */
		while( size + miRequestCurrentSize >= miRequestCurrentCapacity ) {
			miRequestCurrentCapacity *= 2;
			bFlag = true;
		}
		if( bFlag ) {
			char *newBuffer = new char[miRequestCurrentCapacity];
			if( mpRequestBuffer != NULL ) {
				memcpy(newBuffer, mpRequestBuffer, miRequestCurrentSize);
				delete[] mpRequestBuffer;
				mpRequestBuffer = NULL;
			}
			mpRequestBuffer = newBuffer;
		}
		memcpy(mpRequestBuffer + miCurrentSize, buf, size);
		miCurrentSize += size;
		mpRequestBuffer[miCurrentSize] = '\0';
	}
	return true;
}
