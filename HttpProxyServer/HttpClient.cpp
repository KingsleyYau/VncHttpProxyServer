/*
 * HttpClient.cpp
 *
 *  Created on: 2014-12-24
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "HttpClient.h"
#include "LogManager.h"
#include <common/KMutex.h>

#define DWONLOAD_TIMEOUT 30

CURLSH *sh;

void HttpClient::Init() {
	curl_global_init(CURL_GLOBAL_ALL);
	curl_version_info_data *data = curl_version_info(CURLVERSION_FIRST);

	if( data->version != NULL ) {
		printf("# Init( curl_version : %s )\n", data->version);
	}

	if( data->ssl_version != NULL ) {
		printf("# Init( ssl_version : %s )\n", data->ssl_version);
	}

//	sh = curl_share_init();
//	curl_share_setopt(sh, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
}

HttpClient::HttpClient() {
	// TODO Auto-generated constructor stub
	mpIHttpClientCallback = NULL;
	mpCURL = NULL;
	mUrl = "";
	mContentType = "";
}

HttpClient::~HttpClient() {
	// TODO Auto-generated destructor stub
}

void HttpClient::SetCallback(IHttpClientCallback *callback){
	mpIHttpClientCallback = callback;
}

void HttpClient::Stop() {
//	printf("# HttpClient::Stop( this : %p ) \n", this);

	mUrl = "";
	mbStop = true;
}

bool HttpClient::Request(const string& url, const list<string> headers, bool bPost) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"HttpClient::Request( "
			"[http请求], "
			"url : %s "
			")",
			url.c_str()
			);

	bool bFlag = true;

	CURLcode res;

	mContentType = "";
	mContentLength = -1;

	mbStop = false;
	mdUploadTotal = -1;
	mdUploadLast = -1;
	mdUploadLastTime = -1;
	mdDownloadTotal = -1;
	mdDownloadLast = -1;
	mdDownloadLastTime = -1;

	if( mpCURL == NULL ) {
		mpCURL = curl_easy_init();
	}

//	curl_easy_setopt(mpCURL, CURLOPT_SHARE, sh);

	curl_easy_setopt(mpCURL, CURLOPT_URL, url.c_str());

	curl_easy_setopt(mpCURL, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	curl_easy_setopt(mpCURL, CURLOPT_HEADER, 1L);

	if( bPost ) {
		curl_easy_setopt(mpCURL, CURLOPT_POST, 1);
	}

	// 处理send
	curl_easy_setopt(mpCURL, CURLOPT_READFUNCTION, CurlReadHandle);
	curl_easy_setopt(mpCURL, CURLOPT_READDATA, this);

	// 处理http body write函数
//	curl_easy_setopt(mpCURL, CURLOPT_HEADER, this);
	curl_easy_setopt(mpCURL, CURLOPT_WRITEFUNCTION, CurlWriteHandle);
	curl_easy_setopt(mpCURL, CURLOPT_WRITEDATA, this);

	// deal with progress
	curl_easy_setopt(mpCURL, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(mpCURL, CURLOPT_PROGRESSFUNCTION, CurlProgress);
	curl_easy_setopt(mpCURL, CURLOPT_PROGRESSDATA, this);

	//	curl_easy_setopt(mpCURL, CURLOPT_FOLLOWLOCATION, 1);
	// 设置连接超时
	curl_easy_setopt(mpCURL, CURLOPT_CONNECTTIMEOUT, 20L);
//	// 设置执行超时
//	curl_easy_setopt(mpCURL, CURLOPT_TIMEOUT, 30L);
	// 设置不抛退出信号量
	curl_easy_setopt(mpCURL, CURLOPT_NOSIGNAL, 1L);

	struct curl_slist* pList = NULL;
	for( list<string>::const_iterator itr = headers.begin(); itr != headers.end(); itr++ ) {
		string header = *itr;
		pList = curl_slist_append(pList, header.c_str());
//		printf("# HttpClient::Request( Add header : [%s] ) \n", header.c_str());
//		LogManager::GetLogManager()->Log(
//				LOG_MSG,
//				"HttpClient::Request( "
//				"Add header : [%s] "
//				")",
//				header.c_str()
//				);
	}

	if( pList != NULL ) {
		curl_easy_setopt(mpCURL, CURLOPT_HTTPHEADER, pList);
	}

//	printf("# HttpClient::Request( this : %p, curl_easy_perform ) \n", this);
	res = curl_easy_perform(mpCURL);

	double totalTime = 0;
	curl_easy_getinfo(mpCURL, CURLINFO_TOTAL_TIME, &totalTime);
//	printf("# HttpClient::Request( this : %p, totalTime : %f second ) \n", this, totalTime);

	if( mpCURL != NULL ) {
		curl_easy_cleanup(mpCURL);
		mpCURL = NULL;
	}

	if( pList != NULL ) {
		curl_slist_free_all(pList);
	}

	bFlag = (res == CURLE_OK);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"HttpClient::Request( "
			"[http请求, 完成], "
			"res : %d, "
			"url : %s "
			")",
			res,
			url.c_str()
			);

	return bFlag;
}

size_t HttpClient::CurlWriteHandle(void *buffer, size_t size, size_t nmemb, void *data) {
	HttpClient *client = (HttpClient *)data;
	client->HttpWriteHandle(buffer, size, nmemb);
	return size * nmemb;
}

void HttpClient::HttpWriteHandle(void *buffer, size_t size, size_t nmemb) {
//	printf("# HttpClient::HttpWriteHandle( this : %p, size : %d , nmemb : %d ) \n", this, (int)size, (int)nmemb);
//	if( mContentType.length() == 0 ) {
//		char *ct = NULL;
//		CURLcode res = curl_easy_getinfo(mpCURL, CURLINFO_CONTENT_TYPE, &ct);
//
//		if( (res == CURLE_OK) ) {
//			if (NULL != ct) {
//				mContentType = ct;
//			}
//		}
//
//		res = curl_easy_getinfo(mpCURL, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &mContentLength);
//		if (res != CURLE_OK) {
//			mContentLength = -1;
//		}
//	}

	int len = size * nmemb;
	if( mpIHttpClientCallback ) {
		mpIHttpClientCallback->onReceiveBody(this, (const char*)buffer, len);
	}
}

size_t HttpClient::CurlReadHandle(void *buffer, size_t size, size_t nmemb, void *data) {
	HttpClient *client = (HttpClient *)data;
	return client->HttpReadHandle(buffer, size, nmemb);
}

int HttpClient::HttpReadHandle(void *buffer, size_t size, size_t nmemb) {
	int len = size * nmemb;
	if( mpIHttpClientCallback ) {
		return mpIHttpClientCallback->onSendBody(this, (char*)buffer, len);
	}

	return 0;
}

size_t HttpClient::CurlProgress(void *data, double downloadTotal, double downloadNow, double uploadTotal, double uploadNow) {
	HttpClient *client = (HttpClient *)data;
	return client->HttpProgress(downloadTotal, downloadNow, uploadTotal, uploadNow);
}

size_t HttpClient::HttpProgress(double downloadTotal, double downloadNow, double uploadTotal, double uploadNow) {
	double totalTime = 0;
	curl_easy_getinfo(mpCURL, CURLINFO_TOTAL_TIME, &totalTime);

	// mark the upload progress
	mdUploadTotal = uploadTotal;
	mdUploadLast = uploadNow;

	// waiting for upload finish, no upload timeout
	if( uploadNow == uploadTotal ) {
		if( downloadTotal != downloadNow && mdDownloadLast != -1 &&  mdDownloadLast == downloadNow ) {
			if( totalTime - mdDownloadLastTime > DWONLOAD_TIMEOUT ) {
				// DWONLOAD_TIMEOUT no receive data, download timeout
				mbStop = true;
			}
		} else {
			// mark the download progress
			mdDownloadLast = downloadNow;
			mdDownloadLastTime = totalTime;
		}
	}

	return mbStop;
}
