/*
 * HttpProxyServer.h
 *
 *  Created on: 2015-11-14
 *      Author: Max
 */

#ifndef HttpProxyServer_H_
#define HttpProxyServer_H_

#include "TcpProxyClient.h"
#include "DataHttpParser.h"
#include "HttpRequest.h"

#include <common/KSafeMap.h>

#include <curl/curl.h>

typedef KSafeMap<int, DataHttpParser*> DataHttpParserMap;

class HttpProxyServer : public TcpProxyClientCallback, public IHttpRequestCallback {
public:
	HttpProxyServer();
	virtual ~HttpProxyServer();

	bool Run();

private:

	void onSuccess(HttpRequest* request, const char* buf, int size);
	void onFail(HttpRequest* request);
	void onReceiveBody(HttpRequest* request, const char* buf, int size);

	/**
	 * Implement from TcpProxyClientCallback
	 */
	void OnConnected(TcpProxyClient* client);
	void OnDisConnected(TcpProxyClient* client);
	void OnRecvProxyBuffer(TcpProxyClient* client, int seq, int fd, const char* buffer, int len);
	void OnRecvProxyDisconnect(TcpProxyClient* client, int seq, int fd);

	TcpProxyClient mTcpProxyClient;

	DataHttpParserMap mDataHttpParserMap;
};

#endif /* HTTPPROXYSERVER_H_ */
