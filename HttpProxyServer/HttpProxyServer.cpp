/*
 * HttpProxyServer.cpp
 *
 *  Created on: 2015-11-14
 *      Author: Max
 */

#include "HttpProxyServer.h"

HttpProxyServer::HttpProxyServer() {
	// TODO Auto-generated constructor stub
	HttpClient::Init();
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

//	// 直接返回200
//	char respond[1024];
//	sprintf(respond, "HTTP/1.1 200 OK\r\nContext-Length:%d\r\nConnection:Close\r\n\r\n%s\n", strlen("<html>hello world</html>"), "<html>hello world</html>");
//	// 返回代理请求
//	bool bFlag = mTcpProxyClient.SendProxyBuffer(seq, fd, respond, strlen(respond));
//	if( bFlag ) {
//		// 返回断开连接
//		mTcpProxyClient.SendProxyDisconnect(fd);
//	}

	int ret = pDataHttpParser->ParseData(buffer, len);
//	printf("# HttpProxyServer::OnRecvProxyBuffer( pDataHttpParser->ParseData( ret : %d ) ) \n", ret);
	if( ret == 1 ) {
		HttpRequest* request = new HttpRequest;
		request->SetCallback(this);
		request->SetNoCacheData(true);
		request->SetSendData(pDataHttpParser->GetBody(), pDataHttpParser->GetContentLength());
		request->SetFd(fd, seq);
		if( !request->StartRequest(pDataHttpParser->GetUrl(), pDataHttpParser->GetHeaders()) ) {
			delete request;
		}

	} else if( ret == -1 ) {
		// 返回断开连接
		mTcpProxyClient.SendProxyDisconnect(fd);
	}

}

void HttpProxyServer::OnRecvProxyDisconnect(TcpProxyClient* client, int seq, int fd) {
//	printf("# HttpProxyServer::OnRecvProxyDisconnect( fd : %d, seq : %d ) \n", fd, seq);

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

//	LogManager::LogSetFlushBuffer(0);
	LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
	LogManager::GetLogManager()->Start(1000, LOG_STAT, "./log");

	mTcpProxyClient.SetTcpProxyClientCallback(this);
	bFlag = mTcpProxyClient.Start();

	return bFlag;
}

void HttpProxyServer::onSuccess(HttpRequest* request, const char* buf, int size) {
//	printf("# HttpProxyServer::onSuccess( request : %p ) \n", request);

	mDataHttpParserMap.Lock();
	DataHttpParserMap::iterator itr = mDataHttpParserMap.Find(request->Getfd());
	if( itr != mDataHttpParserMap.End() ) {
		// 返回断开连接
		mTcpProxyClient.SendProxyDisconnect(request->Getfd());

	}
	mDataHttpParserMap.Unlock();

	delete request;
}

void HttpProxyServer::onFail(HttpRequest* request) {
//	printf("# HttpProxyServer::onFail( request : %p ) \n", request);
	mDataHttpParserMap.Lock();
	DataHttpParserMap::iterator itr = mDataHttpParserMap.Find(request->Getfd());
	if( itr != mDataHttpParserMap.End() ) {
		// 返回断开连接
		mTcpProxyClient.SendProxyDisconnect(request->Getfd());

	}
	mDataHttpParserMap.Unlock();

	delete request;
}

void HttpProxyServer::onReceiveBody(HttpRequest* request, const char* buf, int size) {
//	printf("# HttpProxyServer::onReceiveBody( request : %p ) \n", request);

	// 返回代理请求
	bool bFlag = mTcpProxyClient.SendProxyBuffer(request->GetSeq(), request->Getfd(), buf, size);
}
