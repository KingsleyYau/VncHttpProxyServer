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
	sprintf(respond, "HTTP/1.1 200 OK\r\nContext-Length:%d\r\nConnection:Close\r\n\r\n%s\n", strlen("<html>hello world</html>"), "<html>hello world</html>");

	DataHttpParser* pDataHttpParser;

	mDataHttpParserMap.Lock();
	DataHttpParserMap::iterator itr = mDataHttpParserMap.Find(fd);
	if( itr == mDataHttpParserMap.End() ) {
		pDataHttpParser = new DataHttpParser();
		mDataHttpParserMap.Insert(fd, pDataHttpParser);
	} else {
		pDataHttpParser = itr->second;
	}
	mDataHttpParserMap.Unlock();

//	int ret = pDataHttpParser->ParseData(buffer, len);

	// 返回代理请求
	bool bFlag = mTcpProxyClient.SendProxyBuffer(seq, fd, respond, strlen(respond));
	if( bFlag ) {
		// 返回断开连接
		mTcpProxyClient.SendProxyDisconnect(fd);
	}
}

void HttpProxyServer::OnRecvProxyDisconnect(TcpProxyClient* client, int seq, int fd) {
	mDataHttpParserMap.Lock();
	DataHttpParserMap::iterator itr = mDataHttpParserMap.Find(fd);
	if( itr != mDataHttpParserMap.End() ) {
		DataHttpParser* pDataHttpParser = itr->second;
		if( pDataHttpParser != NULL ) {
			delete pDataHttpParser;
			pDataHttpParser = NULL;
		}
		mDataHttpParserMap.Erase(itr);
	}
	mDataHttpParserMap.Unlock();
}

bool HttpProxyServer::Run() {
	bool bFlag = false;

	mTcpProxyClient.SetTcpProxyClientCallback(this);
	bFlag = mTcpProxyClient.Start();

	return bFlag;
}
