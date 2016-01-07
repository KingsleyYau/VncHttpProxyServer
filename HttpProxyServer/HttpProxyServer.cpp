/*
 * HttpProxyServer.cpp
 *
 *  Created on: 2015-11-14
 *      Author: Max
 */

#include "HttpProxyServer.h"

HttpProxyServer::HttpProxyServer() {
	// TODO Auto-generated constructor stub
}

HttpProxyServer::~HttpProxyServer() {
	// TODO Auto-generated destructor stub
}

void HttpProxyServer::OnConnected(TcpProxyClient* client) {

}

void HttpProxyServer::OnDisConnected(TcpProxyClient* client) {

}

void HttpProxyServer::OnRecvProxyBuffer(TcpProxyClient* client, int seq, int fd, const char* buffer, int len) {
	// 代理Http请求
	// 直接返回200
	char respond[1024];
	sprintf(respond, "HTTP/1.1 200 OK\r\nContext-Length:%d\r\n\r\n%s", strlen("<html>hello world</html>"), "<html>hello world</html>");

//	DataHttpParser dataHttpParser;

	mTcpProxyClient.SendProxyBuffer(true, seq, fd, respond, strlen(respond));
}

bool HttpProxyServer::Run() {
	bool bFlag = false;

	mTcpProxyClient.SetTcpProxyClientCallback(this);
	bFlag = mTcpProxyClient.Start();

	return bFlag;
}
