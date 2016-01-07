/*
 * HttpProxyServer.h
 *
 *  Created on: 2015-11-14
 *      Author: Max
 */

#ifndef HttpProxyServer_H_
#define HttpProxyServer_H_

#include "TcpProxyClient.h"

class HttpProxyServer : public TcpProxyClientCallback {
public:
	HttpProxyServer();
	virtual ~HttpProxyServer();

	bool Run();

private:

	/**
	 * Implement from TcpProxyClientCallback
	 */
	void OnConnected(TcpProxyClient* client);
	void OnDisConnected(TcpProxyClient* client);
	void OnRecvProxyBuffer(TcpProxyClient* client, int seq, const char* buffer, int len);

	TcpProxyClient mTcpProxyClient;

};

#endif /* HTTPPROXYSERVER_H_ */
