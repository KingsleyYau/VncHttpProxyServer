/*
 * HttpRequest.h
 *
 *  Created on: 2015-2-27
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

#include "HttpClient.h"
#include "HttpRequestDefine.h"

#include <common/KThread.h>

#define MAX_RESPONED_BUFFER 4 * 1024

class HttpRequestRunnable;
class HttpRequest;
class IHttpRequestCallback {
public:
	virtual ~IHttpRequestCallback(){};
	virtual void onSuccess(HttpRequest* request, const char* buf, int size) = 0;
	virtual void onFail(HttpRequest* request) = 0;
	virtual void onReceiveBody(HttpRequest* request, const char* buf, int size) = 0;
};

class HttpRequest : public IHttpClientCallback {
	friend class HttpRequestRunnable;
public:
	HttpRequest();
	virtual ~HttpRequest();

	long StartRequest(const string& url, const list<string> headers, bool bPost = false);
	void StopRequest(bool bWait = false);
	void SetCallback(IHttpRequestCallback *callback);

	// 设置是否缓存返回结果, 默认是缓存
	void SetNoCacheData(bool bCache = true);
	void SetSendData(const char* buffer, int len);

	void SetFd(int fd, int seq);
	int Getfd();
	int GetSeq();

protected:
	void onReceiveBody(HttpClient* client, const char* buf, int size);
	int onSendBody(HttpClient* client, char* buf, int size);

private:
	void InitRespondBuffer();
	bool AddRespondBuffer(const char* buf, int size);

	HttpClient mHttpClient;
	KThread mKThread;
	HttpRequestRunnable* mpHttpRequestRunnable;
	IHttpRequestCallback* mpIHttpRequestCallback;

	string mUrl;
	bool mbPost;
	list<string> mHeaders;

	char* mpRespondBuffer;
	int miCurrentSize;
	int miCurrentCapacity;
	bool mbCache;

	void InitRequestBuffer();
	bool AddRequestBuffer(const char* buf, int size);
	char* mpRequestBuffer;
	int miRequestCurrentSize;
	int miRequestCurrentCapacity;
	int miRequestCurrentSend;

	int fd, seq;
};

#endif /* HTTPREQUEST_H_ */
